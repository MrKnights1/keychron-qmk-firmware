#include "modes.h"

modes_t modes_set_spam(modes_t modes, bool on) {
    modes.spam = on;
    if (on) modes.pilot = false;
    return modes;
}

modes_t modes_set_pilot(modes_t modes, bool on) {
    modes.pilot = on;
    if (on) modes.spam = false;
    return modes;
}

modes_t modes_on_default_layer(modes_t modes, bool windows) {
    if (!windows) {
        modes.spam  = false;
        modes.pilot = false;
    }
    return modes;
}
