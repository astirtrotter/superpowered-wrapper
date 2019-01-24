//
// Created by Yonis Larsson on 2/22/18.
//

#ifndef Header_SPState
#define Header_SPState

enum SPState { None, Initialized, Doing, Paused };

static inline const char *getStateName(SPState state) {
    static const char *stateNames[] = { "None", "Initialized", "Doing", "Paused"};
    return stateNames[state];
}

#endif
