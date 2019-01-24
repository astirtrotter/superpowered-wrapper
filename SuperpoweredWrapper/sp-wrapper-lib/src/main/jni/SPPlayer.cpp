#include "SPPlayer.h"
#include <SLES/OpenSLES.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <malloc.h>
#include <SuperpoweredSimple.h>
#include <SuperpoweredCPU.h>
#include <android/log.h>
#include <base/SPSharedVariables.h>

SPState SPPlayer::state = SPState::None;
SuperpoweredAndroidAudioIO *SPPlayer::audioSystem = NULL;
SuperpoweredAdvancedAudioPlayer* SPPlayer::player1 = NULL;
SuperpoweredAdvancedAudioPlayer* SPPlayer::player2 = NULL;
float SPPlayer::volume1 = 1;
float SPPlayer::volume2 = 1;
bool SPPlayer::duoPlaying = false;

JavaVM* SPPlayer::javaVM = NULL;
jclass SPPlayer::cls = NULL;

float *stereoBuffer = NULL;
int loadedCount;

static void playerEventCallback(void *clientData, SuperpoweredAdvancedAudioPlayerEvent event,
                                void *value) {
    SuperpoweredAdvancedAudioPlayer *player = *((SuperpoweredAdvancedAudioPlayer **) clientData);

    switch (event) {
        case SuperpoweredAdvancedAudioPlayerEvent_LoadSuccess:
            // already released
            if (SPPlayer::state == SPState::Initialized) {
                player->setBpm(0);
                player->setFirstBeatMs(0);
                player->setPosition(player->firstBeatMs, false, false);

                if (++loadedCount == (SPPlayer::duoPlaying ? 2 : 1)) {
                    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Successfully prepared.");
                    SPPlayer::setState(SPState::Paused);
                }
            }
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoadError:
            __android_log_print(ANDROID_LOG_WARN, "SPPlayer", "Failed to load. (%s)",
                                (char *) value);
            SPPlayer::release();
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_EOF:
            __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Playing completed.");

            SPSharedVariables::unlockMutex(); // due to audioProcessing()
            SPPlayer::stop();
            SPSharedVariables::lockMutex();
            if (SPPlayer::javaVM != NULL) {
                // call java onPlayCompleted listener
                JNIEnv *env;
                SPPlayer::javaVM->AttachCurrentThread(&env, NULL);
                jmethodID methodId = env->GetStaticMethodID(SPPlayer::cls, "onPlayCompleted", "()V");
                env->CallStaticVoidMethod(SPPlayer::cls, methodId);
                SPPlayer::javaVM->DetachCurrentThread();
            }
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_DurationChanged:
            __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Duration changed.");
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_HLSNetworkError:
            __android_log_print(ANDROID_LOG_ERROR, "SPPlayer", "HLS network error. (%s)",
                                (char *) value);
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_JogParameter:
            __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Jog parameter. (%s)",
                                (char *) value);
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoopStartReverse:
            __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Loop start reverse. (%s)",
                                (char *) value);
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_LoopEnd:
            __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Loop end. (%s)",
                                (char *) value);
            break;
        case SuperpoweredAdvancedAudioPlayerEvent_ProgressiveDownloadError:
            __android_log_print(ANDROID_LOG_ERROR, "SPPlayer",
                                "Progressive download error. (%s)", (char *) value);
            break;
    }
};

static bool audioProcessing(void *clientdata, short int *audioIO, int numberOfSamples,
                            int __unused samplerate) {

    SPSharedVariables::lockMutex();

    bool silence1 = !SPPlayer::player1->process(stereoBuffer, false, (unsigned int) numberOfSamples, SPPlayer::volume1);
    bool silence2 = true;
    if (SPPlayer::duoPlaying) {
        silence2 = !SPPlayer::player2->process(stereoBuffer, true, (unsigned int) numberOfSamples, SPPlayer::volume2);
    }

    // The stereoBuffer is ready now, let's put the finished audio into the requested buffers.
    if (!silence1 || !silence2) {
        SuperpoweredFloatToShortInt(stereoBuffer, audioIO, (unsigned int) numberOfSamples);
    }

    SPSharedVariables::unlockMutex();

    return !silence1 || !silence2;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SPPlayer::setState(SPState newState) {
    // log: oldState -> newState
    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "state: %s -> %s", getStateName(state), getStateName(newState));
    state = newState;
}

bool SPPlayer::init(const char *tempFolderPath, unsigned int samplerate, unsigned int buffersize) {
    if (!checkActionWithState(SPAction::Init)) return false;
    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "tempFolderPath: %s, samplerate: %i, buffersize: %i", tempFolderPath, samplerate, buffersize);

    SPSharedVariables::initMutex();

    SuperpoweredAdvancedAudioPlayer::setTempFolder(tempFolderPath);
    stereoBuffer = (float *) memalign(16, (buffersize + 16) * sizeof(float) * 2);
    player1 = new SuperpoweredAdvancedAudioPlayer(&player1, playerEventCallback, samplerate, 0);
    player2 = new SuperpoweredAdvancedAudioPlayer(&player2, playerEventCallback, samplerate, 0);
    player1->syncMode = player2->syncMode = SuperpoweredAdvancedAudioPlayerSyncMode_TempoAndBeat;

    audioSystem = new SuperpoweredAndroidAudioIO(samplerate, buffersize, false, true,
                                                       audioProcessing, NULL, -1,
                                                       SL_ANDROID_STREAM_MEDIA, buffersize * 2);

    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Successfully initialized.");

    setState(SPState::Initialized);

    return true;
}

bool SPPlayer::prepare(const char *path1, const char *path2) {
    if (!checkActionWithState(SPAction::Prepare)) return false;

    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "path1: %s, path2: %s", path1, path2);
    loadedCount = 0;
    player1->open(path1);
    if ((duoPlaying = path2 != NULL)) {
        player2->open(path2);
    }

    // log(sucessfully load) will be called automatically by playerEventCallback func.
    // setState(SPState::Paused) will be called automatically by playerEventCallback func.

    return true;
}

bool SPPlayer::seek(double ms) {
    if (!checkActionWithState(SPAction::Seek)) return false;

    if (ms < 0) ms = 0;
    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "ms: %f", ms);

    player1->setPosition(ms, false, false);
    if (duoPlaying) {
        player2->setPosition(ms, false, false);
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Successfully seeked.");

    // no state changes

    return true;
}

bool SPPlayer::play() {
    if (!checkActionWithState(SPAction::Start)) return false;

    SPSharedVariables::lockMutex();

    player1->play(false);
    if (duoPlaying) {
        player2->play(false);
    }
    SuperpoweredCPU::setSustainedPerformanceMode(true); // <-- Important to prevent audio dropouts.

    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Successfully played.");

    setState(SPState::Doing);

    SPSharedVariables::unlockMutex();

    return true;
}

bool SPPlayer::pause() {
    if (!checkActionWithState(SPAction::Pause)) return false;

    SPSharedVariables::lockMutex();

    player1->pause();
    if (duoPlaying) {
        player2->pause();
    }
    SuperpoweredCPU::setSustainedPerformanceMode(false); // <-- Important to prevent audio dropouts.

    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Successfully paused.");

    setState(SPState::Paused);

    SPSharedVariables::unlockMutex();

    return true;
}

bool SPPlayer::stop() {
    if (!checkActionWithState(SPAction::Stop)) return false;

    pause();
    seek(0);

    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Successfully stoped.");

    // no state changes

    return true;
}

bool SPPlayer::release() {
    if (!checkActionWithState(SPAction::Release)) return false;
    if (state == SPState::None) return true;

    setState(SPState::None);

    SPSharedVariables::destoryMutex();
    delete audioSystem;
    delete player1;
    delete player2;
    free(stereoBuffer);
    SuperpoweredAdvancedAudioPlayer::clearTempFolder();

    audioSystem = NULL;
    player1 = NULL;
    player2 = NULL;
    stereoBuffer = NULL;

    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Successfully released.");

    return true;
}

bool SPPlayer::checkActionWithState(SPAction action) {
//    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Try action: %s", getActionName(action));

    bool ret;
    switch (state) {
        case SPState::None:
            ret = action == SPAction::Init ||
                  action == SPAction::Release;
            break;

        case SPState::Initialized:
            ret = action == SPAction::Prepare ||
                  action == SPAction::Release;
            break;

        case SPState::Paused:
            ret = action == SPAction::Start ||
                  action == SPAction::Pause ||
                  action == SPAction::Seek ||
                  action == SPAction::Stop ||
                  action == SPAction::Release ||
                  action == SPAction::CHECK_DOING ||
                  action == SPAction::GET_DURATION ||
                  action == SPAction::GET_POSITION;
            break;

        case SPState::Doing:
            ret = action == SPAction::Pause ||
                  action == SPAction::Seek ||
                  action == SPAction::Stop ||
                  action == SPAction::Release ||
                  action == SPAction::CHECK_DOING ||
                  action == SPAction::GET_DURATION ||
                  action == SPAction::GET_POSITION;
            break;

        default:
            ret = false;
    }

    if (!ret) {
        __android_log_print(ANDROID_LOG_WARN, "SPPlayer", "IllegalStateException: state=%s, action=%s", getStateName(state), getActionName(action));
    }

    return ret;
}

////////////////////////////////////////////////////////////////////////////////////////////////////
// Getter & Setter
////////////////////////////////////////////////////////////////////////////////////////////////////

float SPPlayer::getVolume(int index) {
    return index == 0 ? volume1 : volume2;
}

void SPPlayer::setVolume(int index, float volume) {
    if (index == 0) {
        volume1 = volume;
    } else {
        volume2 = volume;
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Volume: %f, %f.", volume1, volume2);
}

int SPPlayer::getPitch() {
    if (duoPlaying && player2 != NULL) {
        return player2->pitchShift;
    }
    return 0;
}

bool SPPlayer::setPitch(int pitch) {
    if (duoPlaying && player2 != NULL) {
        player2->setPitchShift(pitch);
        return true;
    }
    return false;
}

bool SPPlayer::playing() {
    if (!checkActionWithState(SPAction::CHECK_DOING)) return false;

    bool ret = player1->playing;
    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Playing: %s.", ret ? "true" : "false");

    return ret;
}

unsigned int SPPlayer::duration() {
    if (!checkActionWithState(SPAction::GET_DURATION)) return 0;

    unsigned int ret = player1->durationMs;
    if (duoPlaying && player2->durationMs < ret) {
        ret = player2->durationMs;
    }
//    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Duration: %i", ret);

    return ret;
}

double SPPlayer::position() {
    if (!checkActionWithState(SPAction::GET_POSITION)) return 0;

    double ret = player1->positionMs;
//    __android_log_print(ANDROID_LOG_VERBOSE, "SPPlayer", "Position: %f", ret);

    return ret;
}