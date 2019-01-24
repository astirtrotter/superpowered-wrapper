#include "SPSharedVariables.h"

pthread_mutex_t SPSharedVariables::mutex;
int SPSharedVariables::mutex_count = 0;

void SPSharedVariables::initMutex(bool player) {
    if (mutex_count == 0) {
        pthread_mutex_init(&mutex, NULL);
    }
    mutex_count++;
}

void SPSharedVariables::lockMutex(bool player) {
//    if (player) {
        pthread_mutex_lock(&mutex);
}

void SPSharedVariables::unlockMutex(bool player) {
    pthread_mutex_unlock(&mutex);
}

void SPSharedVariables::destoryMutex(bool player) {
    mutex_count--;
    if (mutex_count == 0) {
        pthread_mutex_destroy(&mutex);
    }
}