// Copyright 2022 Ivan Pointer (@ivanpointer)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

#define XXX KC_NO

#define LAYOUT( \
    L00, L01, L02, L03, L04, L05,                    R00, R01, R02, R03, R04, R05, \
    L10, L11, L12, L13, L14, L15,                    R10, R11, R12, R13, R14, R15, \
    L20, L21, L22, L23, L24, L25,                    R20, R21, R22, R23, R24, R25, \
    L30, L31, L32, L33, L34, L35,                    R30, R31, R32, R33, R34, R35, \
              L36, L26, L16, L06,                    R36, R26, R16, R06, \
              L07, L17, L27, L37,                    R17, R07, \
) { \
    { L00, L01, L02, L03, L04, L05, L06, L07 }, \
    { L10, L11, L12, L13, L14, L15, L16, L17 }, \
    { L20, L21, L22, L23, L24, L25, L26, L27 }, \
    { L30, L31, L32, L33, L34, L35, L36, L37 }, \
\
    { R00, R01, R02, R03, R04, R05, R06, R07 }, \
    { R10, R11, R12, R13, R14, R15, R16, R17 }, \
    { R20, R21, R22, R23, R24, R25, R26, XXX }, \
    { R30, R31, R32, R33, R34, R35, R36, XXX }, \
}

enum custom_keycodes {
    KC_SCROLL = SAFE_RANGE,
    KC_CPI_1,
    KC_CPI_2,
    KC_CPI_3
};

typedef union {
  uint32_t raw;
  struct {
    uint16_t cpi;
  };
} config_pterosphera_t;
