// Copyright 2022 Ivan Pointer (@ivanpointer)
// SPDX-License-Identifier: GPL-2.0-or-later

#pragma once

#include "config.h"

/*
 * Keyboard Matrix Assignments
 *
 * Change this to how you wired your keyboard
 * COLS: AVR pins used for columns, left to right
 * ROWS: AVR pins used for rows, top to bottom
 * DIODE_DIRECTION: COL2ROW = COL = Anode (+), ROW = Cathode (-, marked on diode)
 *                  ROW2COL = ROW = Anode (+), COL = Cathode (-, marked on diode)
 *
 */
#define MATRIX_ROW_PINS { D4, C6, D7, E6 }
#define MATRIX_COL_PINS { B5, B4, F1, C7, D5, B7, D3, D2 }

#define PMW3389_CS_PIN F7

#define RGB_DI_PIN B6
#define RGBLED_NUM 16
#define RGBLED_SPLIT { 8, 8 }
