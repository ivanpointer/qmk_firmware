#include QMK_KEYBOARD_H
#include "split_util.h"

#define LCLK MS_BTN1
#define RCLK MS_BTN2
#define MCLK MS_BTN3
#define MBCK MS_BTN4
#define MFWD MS_BTN5

// Placeholders for DPI adjustments for trackball
#define CPI_UP   KC_NO
#define CPI_DOWN KC_NO
#define CPI_DFLT KC_NO

// Trackball Scroll-Mode placeholder
#define SCRL_MOD KC_NO

enum layers {
    _BASE,
    _SH2,
    _MOU,
    _NAV,
    _NUM,
    _FN1,
    _FN2,
    _SYS
};

enum custom_keycodes {
    STAT_TOGG = SAFE_RANGE,
    STAT_BRID,
    STAT_BRIU,
    STAT_TEST,
};

enum status_error_code {
    STATUS_ERROR_NONE,
    STATUS_ERROR_SPLIT,
    STATUS_ERROR_POINTING,
};

typedef struct {
    uint8_t h;
    uint8_t s;
    uint8_t v;
} status_hsv_t;

typedef struct {
    status_hsv_t primary;
    status_hsv_t secondary;
    bool         alternate;
} status_layer_style_t;

#define STATUS_BRIGHTNESS_STEP 16
#define STATUS_BRIGHTNESS_MIN 16
#define STATUS_BRIGHTNESS_MAX 96
#define STATUS_STARTUP_MS 1200
#define STATUS_STARTUP_STEP_MS 150
#define STATUS_ERROR_ON_MS 160
#define STATUS_ERROR_OFF_MS 160
#define STATUS_ERROR_PAUSE_MS 900
#define STATUS_ERROR_CHECK_MS 250
#define STATUS_ERROR_DISPLAY_MS 30000
#define STATUS_BREATHE_MS 2000
#define STATUS_LAYER_ALTERNATE_MS 700
#define STATUS_BREATHE_MIN 8

static bool     status_enabled          = true;
static uint8_t  status_brightness       = 64;
static uint32_t status_startup_timer    = 0;
static uint32_t status_error_timer      = 0;
static uint32_t status_error_check_timer = 0;
static uint8_t  status_error_code       = STATUS_ERROR_NONE;

static status_hsv_t status_last_hsv = {HSV_OFF};

static const status_layer_style_t status_layer_styles[] = {
    [_BASE] = {{HSV_WHITE}, {HSV_WHITE}, false},
    [_SH2]  = {{HSV_WHITE}, {HSV_BLUE}, true},
    [_MOU]  = {{HSV_CYAN}, {HSV_CYAN}, false},
    [_NAV]  = {{HSV_GREEN}, {HSV_GREEN}, false},
    [_NUM]  = {{HSV_YELLOW}, {HSV_YELLOW}, false},
    [_FN1]  = {{HSV_PURPLE}, {HSV_PURPLE}, false},
    [_FN2]  = {{HSV_PURPLE}, {HSV_CYAN}, true},
    [_SYS]  = {{HSV_PURPLE}, {HSV_YELLOW}, true},
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        KC_ESC  , KC_LBRC , KC_LCBR , KC_RCBR , KC_LPRN , KC_EQUAL, KC_NO,                     KC_NO   , KC_ASTR , KC_RPRN , KC_PLUS , KC_RBRC , KC_EXLM , KC_BSPC,
        MO(_SH2), KC_SCLN , KC_COMM , KC_DOT  , KC_P    , KC_Y    , KC_TAB  ,                     TT(_NUM), KC_F    , KC_G    , KC_C    , KC_R    , KC_L    , MO(_SH2),
        KC_LSFT , KC_A    , KC_O    , KC_E    , KC_U    , KC_I    , KC_NO   ,                     TT(_FN1), KC_D    , KC_H    , KC_T    , KC_N    , KC_S    , KC_RSFT,
        KC_LCTL , KC_QUOT , KC_Q    , KC_J    , KC_K    , KC_X    , TT(_SYS),                     SCRL_MOD, KC_B    , KC_M    , KC_W    , KC_V    , KC_Z    , KC_RCTL,
                            KC_LALT , KC_LGUI , LCLK    ,                                                             TT(_MOU), KC_RGUI , KC_RALT,
                                                RCLK    ,                                                             KC_SPC,
                                                          KC_ENT  ,                                         TT(_FN1),
                                                                    MCLK    ,                     TT(_NAV)
    ),

    [_SH2] = LAYOUT(
        KC_TRNS  , KC_7     , KC_5     , KC_3     , KC_1     , KC_9     , TO(_BASE)    ,                       TO(_BASE), KC_0     , KC_2     , KC_4     , KC_6     , KC_8     , KC_TRNS,
        KC_TRNS  , KC_LT    , KC_GT    , KC_DLR   , KC_AMPR  , KC_MINS  , KC_TRNS  ,                       KC_NO    , KC_SLSH  , KC_BSLS  , KC_AT    , KC_HASH  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_LBRC  , KC_LCBR  , KC_RCBR  , KC_LPRN  , KC_EQL   , KC_NO    ,                       KC_NO    , KC_ASTR  , KC_RPRN  , KC_PLUS  , KC_RBRC  , KC_EXLM  , KC_TRNS,
        KC_TRNS  , KC_QUOT  , KC_NO    , KC_NO    , KC_TILD  , KC_PERC  , KC_NO    ,                       KC_NO    , KC_UNDS  , KC_QUES  , KC_PIPE  , KC_CIRC  , KC_GRV   , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_MOU] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TO(_BASE),                       TO(_BASE), KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS  ,                       KC_NO    , CPI_UP   , MBCK     , KC_NO    , MFWD     , MS_WHLU  , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       KC_NO    , CPI_DOWN , LCLK     , RCLK     , MCLK     , MS_WHLD  , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       SCRL_MOD , CPI_DFLT , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_NAV] = LAYOUT(
        KC_TRNS   , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , TO(_BASE)     ,                         TO(_BASE) , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_TRNS,
        KC_CAPS   , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_TRNS   ,                         KC_NO     , KC_PGUP   , MBCK      , KC_UP     , MFWD      , KC_NO     , KC_TRNS,
        KC_TRNS   , KC_LSFT   , KC_LCTL   , KC_LALT   , KC_LGUI   , KC_NO     , KC_NO     ,                         KC_NO     , KC_PGDN   , KC_LEFT   , KC_DOWN   , KC_RGHT   , KC_NO     , KC_TRNS,
        KC_TRNS   , KC_NO     , LGUI(KC_X), LGUI(KC_C), LGUI(KC_V), KC_NO     , KC_NO     ,                         KC_NO     , KC_NO     , KC_HOME   , KC_NO     , KC_END    , KC_NO     , KC_TRNS,
                                KC_TRNS   , KC_TRNS   , KC_TRNS   ,                                                                         KC_TRNS   , KC_TRNS   , KC_TRNS,
                                                        KC_TRNS   ,                                                                         KC_TRNS,
                                                                    KC_TRNS   ,                                                 KC_TRNS,
                                                                                KC_TRNS   ,                         KC_TRNS
    ),

    [_NUM] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TO(_BASE)    ,                       TO(_BASE), KC_NUM   , KC_PSLS  , KC_PAST  , KC_PMNS  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS  ,                       KC_NO    , KC_P7    , KC_P8    , KC_P9    , KC_PPLS  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_LSFT  , KC_LCTL  , KC_LALT  , KC_LGUI  , KC_NO    , KC_NO    ,                       KC_NO    , KC_P4    , KC_P5    , KC_P6    , KC_PENT  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       KC_NO    , KC_P1    , KC_P2    , KC_P3    , KC_PDOT  , KC_NO    , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_P0    , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_FN1] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TO(_BASE)    ,                       TO(_BASE), KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_F1    , KC_F2    , KC_F3    , KC_F4    , KC_NO    , KC_TRNS  ,                       KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_F5    , KC_F6    , KC_F7    , KC_F8    , KC_NO    , KC_NO    ,                       TT(_FN2) , KC_NO    , KC_RGUI  , KC_RALT  , KC_RCTL  , KC_RSFT  , KC_TRNS,
        KC_TRNS  , KC_F9    , KC_F10   , KC_F11   , KC_F12   , KC_NO    , KC_NO    ,                       KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             TT(_FN2),
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_FN2] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TO(_BASE)    ,                       TO(_BASE), KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_F13   , KC_F14   , KC_F15   , KC_F16   , KC_NO    , KC_TRNS  ,                       KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_F17   , KC_F18   , KC_F19   , KC_F20   , KC_NO    , KC_NO    ,                       KC_NO    , KC_NO    , KC_RGUI  , KC_RALT  , KC_RCTL  , KC_RSFT  , KC_TRNS,
        KC_TRNS  , KC_F21   , KC_F22   , KC_F23   , KC_F24   , KC_NO    , KC_NO    ,                       KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_SYS] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TO(_BASE)    ,                       TO(_BASE), KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS  ,                       KC_NO    , KC_VOLU  , KC_MPRV  , KC_MSTP  , KC_MNXT  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_LSFT  , KC_LCTL  , KC_LALT  , KC_LGUI  , KC_NO    , KC_NO    ,                       KC_NO    , KC_VOLD  , KC_NO    , KC_MPLY  , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       KC_NO    , KC_MUTE  , STAT_BRID, STAT_TEST, STAT_BRIU, STAT_TOGG, KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    )

};

const key_override_t lbrc_to_7 = ko_make_basic(MOD_MASK_SHIFT, KC_LBRC,  KC_7);
const key_override_t lcbr_to_5 = ko_make_basic(MOD_MASK_SHIFT, KC_LCBR,  KC_5);
const key_override_t rcbr_to_3 = ko_make_basic(MOD_MASK_SHIFT, KC_RCBR,  KC_3);
const key_override_t lprn_to_1 = ko_make_basic(MOD_MASK_SHIFT, KC_LPRN,  KC_1);
const key_override_t eql_to_9  = ko_make_basic(MOD_MASK_SHIFT, KC_EQUAL, KC_9);

const key_override_t astr_to_0 = ko_make_basic(MOD_MASK_SHIFT, KC_ASTR,  KC_0);
const key_override_t rprn_to_2 = ko_make_basic(MOD_MASK_SHIFT, KC_RPRN,  KC_2);
const key_override_t plus_to_4 = ko_make_basic(MOD_MASK_SHIFT, KC_PLUS,  KC_4);
const key_override_t rbrc_to_6 = ko_make_basic(MOD_MASK_SHIFT, KC_RBRC,  KC_6);
const key_override_t exlm_to_8 = ko_make_basic(MOD_MASK_SHIFT, KC_EXLM,  KC_8);

const key_override_t *key_overrides[] = {
    &lbrc_to_7,
    &lcbr_to_5,
    &rcbr_to_3,
    &lprn_to_1,
    &eql_to_9,

    &astr_to_0,
    &rprn_to_2,
    &plus_to_4,
    &rbrc_to_6,
    &exlm_to_8,

    NULL,
};

static uint8_t status_wave_value(uint32_t now, uint16_t period, uint8_t min_value) {
    const uint16_t half_cycle = period / 2;
    uint16_t       phase      = now % period;
    uint8_t        max_value  = MAX(status_brightness, min_value);
    uint16_t       travel     = max_value - min_value;

    if (phase > half_cycle) {
        phase = period - phase;
    }

    return min_value + ((uint32_t)travel * phase / half_cycle);
}

static status_layer_style_t status_style_for_active_layer(void) {
    uint8_t layer = get_highest_layer(layer_state | default_layer_state);

    if (layer >= ARRAY_SIZE(status_layer_styles)) {
        layer = _BASE;
    }

    return status_layer_styles[layer];
}

static void status_set_hsv(status_hsv_t color) {
    if (color.h == status_last_hsv.h && color.s == status_last_hsv.s && color.v == status_last_hsv.v) {
        return;
    }

    rgblight_sethsv_noeeprom(color.h, color.s, color.v);
    status_last_hsv = color;
}

static void status_update_error_code(uint32_t now) {
    if (timer_elapsed32(status_error_check_timer) < STATUS_ERROR_CHECK_MS) {
        return;
    }

    status_error_check_timer = now;
    uint8_t next_error       = STATUS_ERROR_NONE;

#ifdef SPLIT_KEYBOARD
    if (!is_transport_connected()) {
        next_error = STATUS_ERROR_SPLIT;
    }
#endif

#ifdef POINTING_DEVICE_ENABLE
    if (next_error == STATUS_ERROR_NONE && pointing_device_get_status() != POINTING_DEVICE_STATUS_SUCCESS) {
        next_error = STATUS_ERROR_POINTING;
    }
#endif

    if (next_error != status_error_code) {
        status_error_code  = next_error;
        status_error_timer = now;
    }
}

static void status_render_startup(void) {
    uint16_t elapsed = timer_elapsed32(status_startup_timer);
    bool     on      = ((elapsed / STATUS_STARTUP_STEP_MS) % 2) == 0;

    status_set_hsv(on ? (status_hsv_t){HSV_WHITE} : (status_hsv_t){HSV_OFF});
}

static void status_render_error(void) {
    uint16_t blink_window = STATUS_ERROR_ON_MS + STATUS_ERROR_OFF_MS;
    uint16_t cycle_ms     = (status_error_code * blink_window) + STATUS_ERROR_PAUSE_MS;
    uint16_t elapsed      = timer_elapsed32(status_error_timer) % cycle_ms;
    bool     on           = false;

    if (elapsed < status_error_code * blink_window) {
        on = (elapsed % blink_window) < STATUS_ERROR_ON_MS;
    }

    status_set_hsv(on ? (status_hsv_t){HSV_RED} : (status_hsv_t){HSV_OFF});
}

static void status_render_normal(uint32_t now) {
    status_layer_style_t style = status_style_for_active_layer();
    status_hsv_t         color = style.primary;

    if (style.alternate && ((now / STATUS_LAYER_ALTERNATE_MS) % 2) == 1) {
        color = style.secondary;
    }

    if (host_keyboard_led_state().caps_lock) {
        color.v = status_wave_value(now, STATUS_BREATHE_MS, STATUS_BREATHE_MIN);
    } else {
        color.v = status_brightness;
    }

    status_set_hsv(color);
}

static void status_render(void) {
    uint32_t now = timer_read32();

    if (!status_enabled) {
        status_set_hsv((status_hsv_t){HSV_OFF});
        return;
    }

    if (timer_elapsed32(status_startup_timer) < STATUS_STARTUP_MS) {
        status_render_startup();
        return;
    }

    status_update_error_code(now);

    if (status_error_code != STATUS_ERROR_NONE && timer_elapsed32(status_error_timer) < STATUS_ERROR_DISPLAY_MS) {
        status_render_error();
        return;
    }

    status_render_normal(now);
}

void keyboard_post_init_user(void) {
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    status_startup_timer     = timer_read32();
    status_error_timer       = status_startup_timer;
    status_error_check_timer = status_startup_timer;
    status_last_hsv          = (status_hsv_t){HSV_OFF};
}

layer_state_t layer_state_set_user(layer_state_t state) {
    status_render();
    return state;
}

bool led_update_user(led_t led_state) {
    (void)led_state;
    status_render();
    return true;
}

void housekeeping_task_user(void) {
    status_render();
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case STAT_TOGG:
            status_enabled = !status_enabled;
            status_render();
            return false;

        case STAT_BRID:
            status_brightness = status_brightness > STATUS_BRIGHTNESS_MIN + STATUS_BRIGHTNESS_STEP ? status_brightness - STATUS_BRIGHTNESS_STEP : STATUS_BRIGHTNESS_MIN;
            status_render();
            return false;

        case STAT_BRIU:
            status_brightness = status_brightness < STATUS_BRIGHTNESS_MAX - STATUS_BRIGHTNESS_STEP ? status_brightness + STATUS_BRIGHTNESS_STEP : STATUS_BRIGHTNESS_MAX;
            status_render();
            return false;

        case STAT_TEST:
            status_startup_timer = timer_read32();
            status_render();
            return false;
    }

    return true;
}
