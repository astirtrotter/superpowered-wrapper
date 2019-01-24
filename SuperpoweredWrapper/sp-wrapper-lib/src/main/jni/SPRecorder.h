#ifndef Header_SPRecorder
#define Header_SPRecorder

#include <math.h>
#include <pthread.h>

#include "base/SPState.h"
#include "base/SPAction.h"
#include <SuperpoweredAdvancedAudioPlayer.h>
#include <SuperpoweredRecorder.h>
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <lame.h>
#include <jni.h>

const int MP3_SIZE = 8192;

class SPRecorder {
public:
    static bool init(const char *tempPath, unsigned int samplerate, unsigned int buffersize, int channels);
    static bool prepare(const char *path);
    static bool record();
    static bool pause();
    static bool stop();
    static bool release();

public:
    static void setState(SPState newState);
    static bool checkActionWithState(SPAction action);

public:
    static SPState state;
    static SuperpoweredAndroidAudioIO *audioSystem;
    static SuperpoweredRecorder *recorder;

    static bool isMp3;
    static lame_global_flags *lame;
    static FILE *mp3;

    static bool wasStarted;
    static char *path;

    static JavaVM* javaVM;
    static jclass cls;
};
#endif