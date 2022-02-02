// Copyright 2022 Ivan Pointer (@ivanpointer)
// SPDX-License-Identifier: GPL-2.0-or-later

// https://docs.google.com/spreadsheets/d/1jgQjds1BM_JUTmnuVx91Nt6bxBOcC_TLZnX9TBECRXk

#include QMK_KEYBOARD_H
#include "stdbool.h"

#define XXX             KC_NO
#define ___	            KC_TRNS
#define KC_LEFT_CURLY   LSFT(KC_LEFT_BRACKET)
#define KC_RIGHT_CURLY  LSFT(KC_RIGHT_BRACKET)
#define KC_LEFT_PAREN	LSFT(KC_9)
#define KC_RIGHT_PAREN	LSFT(KC_0)
#define KC_PLUS	        LSFT(KC_EQUAL)
#define KC_EXCLAIM	    LSFT(KC_1)
#define KC_DOLLAR	    LSFT(KC_4)
#define KC_AMPERSAND	LSFT(KC_7)
#define KC_TILDE	    LSFT(KC_GRAVE)
#define KC_PERCENT	    LSFT(KC_5)
#define KC_UNDERSCORE	LSFT(KC_MINUS)
#define KC_QUESTION	    LSFT(KC_SLASH)
#define KC_PIPE	        LSFT(KC_BACKSLASH)
#define KC_CARET	    LSFT(KC_6)
#define KC_AT	        LSFT(KC_2)
#define KC_POUND	    LSFT(KC_3)
#define KC_MAC_UNDO	    LGUI(KC_Z)
#define KC_MAC_CUT	    LGUI(KC_X)
#define KC_MAC_COPY	    LGUI(KC_C)
#define KC_MAC_PASTE	LGUI(KC_V)

// Defines names for use in layer keycodes and the keymap
enum layer_names {
    _BASE,
    _SHIFT2,
    _ARROWS,
    _NUMPAD,
    _FNKEYS,
    _FNKEYS2,
    _MOUSE
};

bool _myCapsState;
uint8_t _myLayer;

void led_set_user(uint8_t usb_led) {
  _myCapsState = usb_led & (1<<USB_LED_CAPS_LOCK);
    switch(_myCapsState) {
    case true:
        rgblight_mode_noeeprom(RGBLIGHT_MODE_BREATHING + 3);
        break;
    default:
        rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
        break;
    }
}

layer_state_t layer_state_set_user(layer_state_t state) {
    _myLayer = biton32(state);
    switch (_myLayer) {
        case _SHIFT2: // Shifted Layer (Linux)
            rgblight_sethsv(HSV_CYAN);
            break;
        case _ARROWS: // Bone (Mac)
            rgblight_sethsv(HSV_GREEN);
            break;
        case _NUMPAD: // Shifted (Mac)
            rgblight_sethsv(HSV_ORANGE);
            break;
        case _FNKEYS: // Utility
            rgblight_sethsv(HSV_BLUE);
            break;
        case _FNKEYS2: // Utility
            rgblight_sethsv(HSV_WHITE);
            break;
        case _MOUSE: // Utility
            rgblight_sethsv(HSV_ORANGE);
            break;
        default:
            rgblight_sethsv(HSV_RED);
            break;
    }
    return state;
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
[_BASE] = LAYOUT(
            KC_ESCAPE,  KC_LEFT_BRACKET,    KC_LEFT_CURLY,   KC_RIGHT_CURLY,    KC_LEFT_PAREN,         KC_EQUAL,              KC_ASTERISK,   KC_RIGHT_PAREN,          KC_PLUS, KC_RIGHT_BRACKET,       KC_EXCLAIM,     KC_BACKSPACE,
          MO(_SHIFT2),     KC_SEMICOLON,         KC_COMMA,           KC_DOT,             KC_P,             KC_Y,                     KC_F,             KC_G,             KC_C,             KC_R,             KC_L,      MO(_SHIFT2),
        KC_LEFT_SHIFT,             KC_A,             KC_O,             KC_E,             KC_U,             KC_I,                     KC_D,             KC_H,             KC_T,             KC_N,             KC_S,   KC_RIGHT_SHIFT,
         KC_LEFT_CTRL,         KC_QUOTE,             KC_Q,             KC_J,             KC_K,             KC_X,                     KC_B,             KC_M,             KC_W,             KC_V,             KC_Z,    KC_RIGHT_CTRL,
                                              KC_LEFT_ALT,      KC_LEFT_GUI,         KC_ENTER,           KC_TAB,               TT(_MOUSE),         KC_SPACE,     KC_RIGHT_GUI,     KC_RIGHT_ALT,
                                               KC_MS_BTN1,       KC_MS_BTN2,      TT(_ARROWS),      TT(_NUMPAD),                KC_DELETE,      TT(_FNKEYS)
),
[_SHIFT2] = LAYOUT(
            TO(_BASE),             KC_7,             KC_5,             KC_3,             KC_1,             KC_9,                     KC_0,             KC_2,             KC_4,             KC_6,             KC_8,     KC_BACKSPACE,
            TO(_BASE),              XXX,            KC_LT,            KC_GT,        KC_DOLLAR,     KC_AMPERSAND,                 KC_MINUS,         KC_SLASH,     KC_BACKSLASH,            KC_AT,         KC_POUND,        TO(_BASE),
                  XXX,  KC_LEFT_BRACKET,    KC_LEFT_CURLY,   KC_RIGHT_CURLY,    KC_LEFT_PAREN,         KC_EQUAL,              KC_ASTERISK,   KC_RIGHT_PAREN,          KC_PLUS, KC_RIGHT_BRACKET,       KC_EXCLAIM,              XXX,
                  XXX,              XXX,              XXX,              XXX,         KC_TILDE,       KC_PERCENT,            KC_UNDERSCORE,      KC_QUESTION,          KC_PIPE,         KC_CARET,         KC_GRAVE,              XXX,
                                                      XXX,              XXX,              XXX,              XXX,                      XXX,              XXX,              XXX,              XXX,
                                                      XXX,              XXX,              XXX,              XXX,                      XXX,              XXX
),
[_ARROWS] = LAYOUT(
         TO(_BASE),           XXX,           XXX,           XXX,           XXX,           XXX,                   XXX,           XXX,           XXX,           XXX,      KC_PAUSE,     KC_INSERT,
      KC_CAPS_LOCK,           XXX,           XXX,           XXX,           XXX,           XXX,                   XXX,       KC_HOME,         KC_UP,        KC_END,    KC_PAGE_UP,           XXX,
               XXX, KC_LEFT_SHIFT,  KC_LEFT_CTRL,   KC_LEFT_ALT,   KC_LEFT_GUI,           XXX,                   XXX,       KC_LEFT,       KC_DOWN,      KC_RIGHT,  KC_PAGE_DOWN,           XXX,
               XXX,   KC_MAC_UNDO,    KC_MAC_CUT,   KC_MAC_COPY,  KC_MAC_PASTE,           XXX,                   XXX,           XXX,           XXX,           XXX,           XXX,           XXX,
                                             XXX,           XXX,           XXX,           XXX,                   XXX,           XXX,           XXX,           XXX,
                                             XXX,           XXX,     TO(_BASE),           XXX,                   XXX,           XXX
),
[_NUMPAD] = LAYOUT(
          TO(_BASE),            XXX,            XXX,            XXX,            XXX,            XXX,                    XXX,    KC_NUM_LOCK,    KC_KP_SLASH, KC_KP_ASTERISK,    KC_KP_MINUS,   KC_BACKSPACE,
                XXX,            XXX,            XXX,            XXX,            XXX,            XXX,                    XXX,        KC_KP_7,        KC_KP_8,        KC_KP_9,     KC_KP_PLUS,      KC_ESCAPE,
                XXX,  KC_LEFT_SHIFT,   KC_LEFT_CTRL,    KC_LEFT_ALT,    KC_LEFT_GUI,            XXX,                    XXX,        KC_KP_4,        KC_KP_5,        KC_KP_6,            XXX,      KC_DELETE,
                XXX,            XXX,            XXX,            XXX,            XXX,            XXX,                    XXX,        KC_KP_1,        KC_KP_2,        KC_KP_3,            XXX,            XXX,
                                                XXX,            XXX,            XXX,            XXX,            KC_KP_ENTER,        KC_KP_0,            XXX,      KC_KP_DOT,
                                                XXX,            XXX,            XXX,      TO(_BASE),                    XXX,            XXX
),
[_FNKEYS] = LAYOUT(
         TO(_BASE),           XXX,           XXX,           XXX,           XXX,           XXX,                   XXX,           XXX,           XXX,           XXX,           XXX,           XXX,
               XXX,         KC_F1,         KC_F2,         KC_F3,         KC_F4,           XXX,                   XXX,           XXX,           XXX,           XXX,           XXX,           XXX,
               XXX,         KC_F5,         KC_F6,         KC_F7,         KC_F8,           XXX,                   XXX,   KC_LEFT_GUI,   KC_LEFT_ALT,  KC_LEFT_CTRL, KC_LEFT_SHIFT,           XXX,
               XXX,         KC_F9,        KC_F10,        KC_F11,        KC_F12,           XXX,                   XXX,           XXX,           XXX,           XXX,           XXX,           XXX,
                                             XXX,           XXX,           XXX,           XXX,                   XXX,           XXX,           XXX,           XXX,
                                             XXX,           XXX,           XXX,           XXX,          TT(_FNKEYS2),     TO(_BASE)
),
[_FNKEYS2] = LAYOUT(
         TO(_BASE),           XXX,           XXX,           XXX,           XXX,           XXX,                   XXX,           XXX,           XXX,           XXX,           XXX,         RESET,
               XXX,        KC_F13,        KC_F14,        KC_F15,        KC_F16,           XXX,                   XXX,           XXX,           XXX,           XXX,           XXX,           XXX,
               XXX,        KC_F17,        KC_F18,        KC_F19,        KC_F20,           XXX,                   XXX,   KC_LEFT_GUI,   KC_LEFT_ALT,  KC_LEFT_CTRL, KC_LEFT_SHIFT,           XXX,
               XXX,        KC_F21,        KC_F22,        KC_F23,        KC_F24,           XXX,                   XXX,           XXX,           XXX,           XXX,           XXX,           XXX,
                                             XXX,           XXX,           XXX,           XXX,                   XXX,           XXX,           XXX,           XXX,
                                             XXX,           XXX,           XXX,           XXX,             TO(_BASE),           XXX
),
[_MOUSE] = LAYOUT(
         TO(_BASE),           XXX,           XXX,           XXX,           XXX,           XXX,                   XXX,           XXX,           XXX,           XXX,           XXX,           XXX,
               XXX,           XXX,           XXX,           XXX,           XXX,           XXX,              KC_CPI_1,           XXX,           XXX,           XXX,           XXX,           XXX,
               XXX, KC_LEFT_SHIFT,  KC_LEFT_CTRL,   KC_LEFT_ALT,   KC_LEFT_GUI,           XXX,              KC_CPI_2,    KC_MS_BTN1,    KC_MS_BTN2,           XXX,           XXX,           XXX,
               XXX,   KC_MAC_UNDO,    KC_MAC_CUT,   KC_MAC_COPY,  KC_MAC_PASTE,           XXX,              KC_CPI_3,           XXX,    KC_MS_BTN3,           XXX,           XXX,           XXX,
                                             XXX,           XXX,           XXX,           XXX,             TO(_BASE),           XXX,           XXX,           XXX,
                                             XXX,           XXX,           XXX,           XXX,                   XXX,           XXX
)
};
