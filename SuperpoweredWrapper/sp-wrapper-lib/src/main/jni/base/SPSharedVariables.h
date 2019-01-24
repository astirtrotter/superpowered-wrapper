//
// Created by Yonis Larsson on 3/16/18.
//

#ifndef Header_SPSharedVairables
#define Header_SPSharedVairables

#include <pthread.h>

class SPSharedVariables {
public:
    static void initMutex(bool player = true);
    static void lockMutex(bool player = true);
    static void unlockMutex(bool player = true);
    static void destoryMutex(bool player = true);

private:
    static pthread_mutex_t mutex;
    static int mutex_count;
};
#endif
