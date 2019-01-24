#include <android/log.h>
#include <malloc.h>
#include <SuperpoweredSimple.h>
#include <SLES/OpenSLES_AndroidConfiguration.h>
#include <SLES/OpenSLES.h>
#include <string.h>
#include <base/SPSharedVariables.h>
#include "SPRecorder.h"

SPState SPRecorder::state = SPState::None;
SuperpoweredAndroidAudioIO *SPRecorder::audioSystem = NULL;
SuperpoweredRecorder *SPRecorder::recorder = NULL;

bool SPRecorder::isMp3 = false;
lame_global_flags *SPRecorder::lame = NULL;
FILE *SPRecorder::mp3 = NULL;

bool SPRecorder::wasStarted = false;
char *SPRecorder::path = NULL;

JavaVM* SPRecorder::javaVM = NULL;
jclass SPRecorder::cls = NULL;

static float *recorderBuffer = NULL;
static short int *mp3Output = NULL;
static unsigned char mp3Buffer[MP3_SIZE];

static bool audioProcessing(void *clientdata, short int *audioIO, int numberOfSamples,
                            int __unused samplerate) {

    SPSharedVariables::lockMutex(false);

    SuperpoweredShortIntToFloat(audioIO, recorderBuffer, (unsigned int) numberOfSamples);
    if (SPRecorder::javaVM != NULL) {
        // call java onPlayCompleted listener
        float peak = SuperpoweredPeak(recorderBuffer, (unsigned int) numberOfSamples);
//        __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "peak: %f", peak);

        JNIEnv *env;
        SPRecorder::javaVM->AttachCurrentThread(&env, NULL);
        jmethodID methodId = env->GetStaticMethodID(SPRecorder::cls, "onPeakChanged", "(F)V");
        env->CallStaticVoidMethod(SPRecorder::cls, methodId, peak);
        SPRecorder::javaVM->DetachCurrentThread();
    }
    if (SPRecorder::state == SPState::Doing) {
        if (SPRecorder::isMp3) {
            memset(mp3Output, 0, sizeof(mp3Output));
            SuperpoweredFloatToShortInt(recorderBuffer, mp3Output, (unsigned int) numberOfSamples);
            /* LAME */
            int result = lame_encode_buffer_interleaved(SPRecorder::lame, mp3Output, numberOfSamples, mp3Buffer, MP3_SIZE);
            if (result > 0) {
                fwrite(mp3Buffer, (size_t) result, 1, SPRecorder::mp3);
            }
            /* LAME */
        } else {
            SPRecorder::recorder->process(recorderBuffer, NULL,
                                          (unsigned int) numberOfSamples);
        }
    }

    SPSharedVariables::unlockMutex(false);

    return true;
}

////////////////////////////////////////////////////////////////////////////////////////////////////

void SPRecorder::setState(SPState newState) {
    // log: oldState -> newState
    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "state: %s -> %s", getStateName(state), getStateName(newState));
    state = newState;
}

/**
 *
 * @param tempPath  if null, mp3 mode.
 * @param samplerate
 * @param buffersize
 * @return
 */
bool SPRecorder::init(const char *tempPath, unsigned int samplerate, unsigned int buffersize, int channels) {
    if (!checkActionWithState(SPAction::Init)) return false;
    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "samplerate: %i, buffersize: %i", samplerate, buffersize);

    SPSharedVariables::initMutex(false);

    SPRecorder::isMp3 = tempPath == NULL;

    recorderBuffer = (float *) memalign(16, (buffersize + 16) * sizeof(float) * 2);
    mp3Output = (short int *) memalign(16, (buffersize + 16) * sizeof(short int) * 2);

    audioSystem = new SuperpoweredAndroidAudioIO(samplerate, buffersize, true, false,
                                                 audioProcessing, NULL, SL_ANDROID_STREAM_MEDIA,
                                                 -1, buffersize * 2);
    if (isMp3) {
        lame = lame_init();
        lame_set_in_samplerate(lame, samplerate);
        lame_set_num_channels(lame, channels);
        lame_set_out_samplerate(lame, samplerate);
//        lame_set_brate(lame, buffersize);
        lame_set_quality(lame, 7);
        lame_init_params(lame);
    } else {
        recorder = new SuperpoweredRecorder(tempPath, samplerate, 1, (unsigned char) channels);
    }
    wasStarted = false;

    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "Successfully initialized.");

    setState(SPState::Initialized);

    return true;
}

bool SPRecorder::prepare(const char *path) {
    if (!checkActionWithState(SPAction::Prepare)) return false;

    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "path: %s", path);

    SPRecorder::path = (char *) malloc(sizeof(char) * (strlen(path) + 1));
    strcpy(SPRecorder::path, path);

    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "Successfully prepared.");
    setState(SPState::Paused);

    return true;
}

bool SPRecorder::record() {
    if (!checkActionWithState(SPAction::Start)) return false;

    SPSharedVariables::lockMutex(false);

    if (!wasStarted) {
        if (isMp3) {
            __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "create file path: %s", path);
            mp3 = fopen(path, "wb");
            if (mp3 == NULL) {
                __android_log_print(ANDROID_LOG_ERROR, "SPRecorder", "failed to create file path: %s", path);
                SPSharedVariables::unlockMutex(false);
                return false;
            }
        } else {
            if (!recorder->start(path)) {
                SPSharedVariables::unlockMutex(false);
                return false;
            }
        }
        wasStarted = true;
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "Successfully started.");
    setState(SPState::Doing);

    SPSharedVariables::unlockMutex(false);

    return true;
}

bool SPRecorder::pause() {
    if (!checkActionWithState(SPAction::Pause)) return false;

    SPSharedVariables::lockMutex(false);

    if (isMp3) {
        int result = lame_encode_flush(lame, mp3Buffer, MP3_SIZE);
        if (result > 0) {
            fwrite(mp3Buffer, (size_t) result, 1, mp3);
        }
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "Successfully paused.");
    setState(SPState::Paused);

    SPSharedVariables::unlockMutex(false);

    return true;
}

bool SPRecorder::stop() {
    if (!checkActionWithState(SPAction::Stop)) return false;

    SPSharedVariables::lockMutex(false);

    if (state == SPState::Doing) {
        setState(SPState::Paused);
    }

    if (wasStarted) {
        if (isMp3) {
            fclose(mp3);
        } else {
            recorder->stop();
        }
        wasStarted = false;
    }

    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "Successfully stoped.");

    SPSharedVariables::unlockMutex(false);

    return true;
}

bool SPRecorder::release() {
    if (!checkActionWithState(SPAction::Release)) return false;
    if (state == SPState::None) return true;

    setState(SPState::None);

    SPSharedVariables::destoryMutex(false);

    if (wasStarted) {
        if (isMp3) {
            fclose(mp3);
        } else {
            recorder->stop();
        }
        wasStarted = false;
    }

    if (isMp3) {
        lame_close(lame);
        lame = NULL;
        free(mp3Output);
    }

    delete audioSystem;
    delete recorder;
    free(recorderBuffer);
    free(path);

    audioSystem = NULL;
    recorder = NULL;
    path = NULL;

    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "Successfully released.");

    return true;
}

bool SPRecorder::checkActionWithState(SPAction action) {
//    __android_log_print(ANDROID_LOG_VERBOSE, "SPRecorder", "Try action: %s", getActionName(action));

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
                  action == SPAction::Stop ||
                  action == SPAction::Release;
            break;

        case SPState::Doing:
            ret = action == SPAction::Pause ||
                  action == SPAction::Stop ||
                  action == SPAction::Release;
            break;

        default:
            ret = false;
    }

    if (!ret) {
        __android_log_print(ANDROID_LOG_WARN, "SPRecorder", "IllegalStateException: state=%s, action=%s", getStateName(state), getActionName(action));
    }

    return ret;
}