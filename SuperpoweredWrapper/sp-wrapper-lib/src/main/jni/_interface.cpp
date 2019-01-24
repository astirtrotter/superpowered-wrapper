//
// Created by Yonis Larsson on 2/22/18.
//
#include "SPPlayer.h"
#include "SPRecorder.h"
#include <jni.h>

////////////////////////////////////////////////////////////////////////////////////////////////////
// Player
////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_init(JNIEnv * __unused javaEnvironment, jobject __unused obj, jstring tempFolderPath, jint samplerate, jint buffersize) {
    const char *path = javaEnvironment->GetStringUTFChars(tempFolderPath, NULL);
    bool ret = SPPlayer::init(path, (unsigned int) samplerate, (unsigned int) buffersize);
    javaEnvironment->ReleaseStringUTFChars(tempFolderPath, path);

    javaEnvironment->GetJavaVM(&SPPlayer::javaVM);
    jclass cls = javaEnvironment->GetObjectClass(obj);
    SPPlayer::cls = (jclass) javaEnvironment->NewGlobalRef(cls);

    return ret;
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_prepare(JNIEnv * javaEnvironment, jobject obj, jstring filePath1, jstring filePath2) {
    const char *path1 = filePath1 != NULL ? javaEnvironment->GetStringUTFChars(filePath1, NULL) : NULL;
    const char *path2 = filePath2 != NULL ? javaEnvironment->GetStringUTFChars(filePath2, NULL) : NULL;
    bool ret = SPPlayer::prepare(path1, path2);
    if (filePath1 != NULL) javaEnvironment->ReleaseStringUTFChars(filePath1, path1);
    if (filePath2 != NULL) javaEnvironment->ReleaseStringUTFChars(filePath2, path2);

    return ret;
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_play(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPPlayer::play();
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_pause(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPPlayer::pause();
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_stop(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPPlayer::stop();
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_release(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPPlayer::release();
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_seek(JNIEnv * __unused javaEnvironment, jobject __unused obj, jdouble ms) {
    return SPPlayer::seek(ms);
}

extern "C" JNIEXPORT float Java_org_at_sp_SPPlayerWrapper_getVolume(JNIEnv * __unused javaEnvironment, jobject __unused obj, jint index) {
    return SPPlayer::getVolume(index);
}

extern "C" JNIEXPORT void Java_org_at_sp_SPPlayerWrapper_setVolume(JNIEnv * __unused javaEnvironment, jobject __unused obj, jint index, jfloat volume) {
    SPPlayer::setVolume(index, volume);
}

extern "C" JNIEXPORT int Java_org_at_sp_SPPlayerWrapper_getPitch(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPPlayer::getPitch();
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_setPitch(JNIEnv * __unused javaEnvironment, jobject __unused obj, jint pitch) {
    return SPPlayer::setPitch(pitch);
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPPlayerWrapper_playing(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPPlayer::playing();
}

extern "C" JNIEXPORT unsigned int Java_org_at_sp_SPPlayerWrapper_duration(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPPlayer::duration();
}

extern "C" JNIEXPORT double Java_org_at_sp_SPPlayerWrapper_position(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPPlayer::position();
}


////////////////////////////////////////////////////////////////////////////////////////////////////
// Recorder
////////////////////////////////////////////////////////////////////////////////////////////////////

extern "C" JNIEXPORT bool Java_org_at_sp_SPRecorderWrapper_init(JNIEnv * __unused javaEnvironment, jobject __unused obj, jstring tempPath, jint samplerate, jint buffersize, jint channels) {
    const char *path = tempPath != NULL ? javaEnvironment->GetStringUTFChars(tempPath, NULL) : NULL;
    bool ret = SPRecorder::init(path, (unsigned int) samplerate, (unsigned int) buffersize, channels);
    if (tempPath != NULL) javaEnvironment->ReleaseStringUTFChars(tempPath, path);

    javaEnvironment->GetJavaVM(&SPRecorder::javaVM);
    jclass cls = javaEnvironment->GetObjectClass(obj);
    SPRecorder::cls = (jclass) javaEnvironment->NewGlobalRef(cls);

    return ret;
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPRecorderWrapper_prepare(JNIEnv * __unused javaEnvironment, jobject __unused obj, jstring filePath) {
    const char *path = javaEnvironment->GetStringUTFChars(filePath, NULL);
    bool ret = SPRecorder::prepare(path);
    javaEnvironment->ReleaseStringUTFChars(filePath, path);

    return ret;
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPRecorderWrapper_record(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPRecorder::record();
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPRecorderWrapper_pause(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPRecorder::pause();
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPRecorderWrapper_stop(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPRecorder::stop();
}

extern "C" JNIEXPORT bool Java_org_at_sp_SPRecorderWrapper_release(JNIEnv * __unused javaEnvironment, jobject __unused obj) {
    return SPRecorder::release();
}