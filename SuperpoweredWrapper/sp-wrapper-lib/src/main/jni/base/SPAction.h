//
// Created by Yonis Larsson on 2/26/18.
//

#ifndef Header_SPAction
#define Header_SPAction

enum SPAction { Init, Prepare, Start, Pause, Stop, Seek, Release,
    GET_DURATION, CHECK_DOING, GET_POSITION };

static inline const char *getActionName(SPAction action) {
    static const char *actionNames[] = { "Init", "Prepare", "Start", "Pause", "Stop", "Seek", "Release",
        "GetDuration", "CheckDoing", "GetPosition" };
    return actionNames[action];
}

#endif
