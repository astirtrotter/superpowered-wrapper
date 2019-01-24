#ifndef Header_SPPlayer
#define Header_SPPlayer

#include <math.h>
#include <pthread.h>

#include "base/SPState.h"
#include "base/SPAction.h"
#include <SuperpoweredAdvancedAudioPlayer.h>
#include <AndroidIO/SuperpoweredAndroidAudioIO.h>
#include <jni.h>

class SPPlayer {
public:
	static bool init(const char *tempFolderPath, unsigned int samplerate, unsigned int buffersize);
	static bool prepare(const char *path1, const char *path2);
	static bool seek(double ms);
    static bool play();
    static bool pause();
    static bool stop();
    static bool release();

    static void setVolume(int index, float volume);
    static float getVolume(int index);
    static bool setPitch(int pitch);
    static int getPitch();
    static bool playing();
	static unsigned int duration();
	static double position();

public:
	static void setState(SPState newState);
	static bool checkActionWithState(SPAction action);

public:
	static SPState state;
	static SuperpoweredAndroidAudioIO *audioSystem;
	static SuperpoweredAdvancedAudioPlayer *player1;
	static SuperpoweredAdvancedAudioPlayer *player2;
	static float volume1;
	static float volume2;
	static bool duoPlaying;

	static JavaVM* javaVM;
    static jclass cls;
};
#endif