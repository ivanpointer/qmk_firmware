// Copyright 2022 Ivan Pointer (@ivanpointer)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "quantum.h"

#define LAYOUT( \
    L05, L04, L03, L02, L01, L00,                    R00, R01, R02, R03, R04, R05, \
    L15, L14, L13, L12, L11, L10,                    R10, R11, R12, R13, R14, R15, \
    L25, L24, L23, L22, L21, L20,                    R20, R21, R22, R23, R24, R25, \
    L35, L34, L33, L32, L31, L30,                    R30, R31, R32, R33, R34, R35, \
              L36, L26, L16, L06,                    R36, R26, R16, R06, \
              L17, L07, L37, L27,                    R17, R07 \
) { \
    { L00, L01, L02, L03, L04, L05, L06, L07 }, \
    { L10, L11, L12, L13, L14, L15, L16, L17 }, \
    { L20, L21, L22, L23, L24, L25, L26, L27 }, \
    { L30, L31, L32, L33, L34, L35, L36, L37 }, \
\
    { R00, R01, R02, R03, R04, R05, R06, R07 }, \
    { R10, R11, R12, R13, R14, R15, R16, R17 }, \
    { R20, R21, R22, R23, R24, R25, R26, XXX }, \
    { R30, R31, R32, R33, R34, R35, R36, XXX } \
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
