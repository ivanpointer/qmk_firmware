// Copyright 2022 Ivan Pointer (@ivanpointer)
// SPDX-License-Identifier: GPL-2.0-or-later

#include QMK_KEYBOARD_H

#define XXX KC_NO

// Defines names for use in layer keycodes and the keymap
enum layer_names {
    _BASE
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    /* Base */
    [_BASE] = LAYOUT(
        XXX, XXX, XXX, XXX, XXX, XXX,        XXX, XXX, XXX, XXX, XXX, XXX,
        XXX, XXX, XXX, XXX, XXX, XXX,        XXX, XXX, XXX, XXX, XXX, XXX,
        XXX, XXX, XXX, XXX, XXX, XXX,        XXX, XXX, XXX, XXX, XXX, XXX,
        XXX, XXX, XXX, XXX, XXX, XXX,        XXX, XXX, XXX, XXX, XXX, XXX,
        XXX, XXX, XXX, XXX,        XXX, XXX, XXX, XXX,
        XXX, XXX, XXX, XXX,        XXX, XXX
    )
};
