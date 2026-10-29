#pragma once

#include <stdbool.h>

/*
 * Spam and pilot mode are mutually exclusive: both drive keys with
 * register_code, and spam's tap cycle would release a key a pilot binding
 * still holds. Both only run on the Windows layers, where Fn+G/Fn+H can
 * switch them off again.
 */
typedef struct {
    bool spam;
    bool pilot;
} modes_t;

modes_t modes_set_spam(modes_t modes, bool on);
modes_t modes_set_pilot(modes_t modes, bool on);
modes_t modes_on_default_layer(modes_t modes, bool windows);
