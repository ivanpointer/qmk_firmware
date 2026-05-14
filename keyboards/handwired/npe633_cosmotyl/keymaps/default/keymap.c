#include QMK_KEYBOARD_H
#include "split_util.h"

#define LCLK MS_BTN1
#define RCLK MS_BTN2
#define MCLK MS_BTN3
#define MBCK MS_BTN4
#define MFWD MS_BTN5

// Trackball mode aliases used in the layout below.
#define SCRL_MOD TB_L_SMRT
#define CPI_UP   TB_L_BRI
#define CPI_DOWN TB_L_VOL
#define CPI_DFLT TB_L_PAN
#define TB_L_ZOOM_KEY TB_L_ZOOM
#define TB_L_ROT_KEY  TB_L_ROT
#define TB_R_SMRT_KEY TB_R_SMRT
#define TB_R_PAN_KEY  TB_R_PAN

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

    TB_BASE,
    TB_L_SMRT,
    TB_R_SMRT,
    TB_L_VSCR,
    TB_R_VSCR,
    TB_L_HSCR,
    TB_R_HSCR,
    TB_L_PAN,
    TB_R_PAN,
    TB_L_VOL,
    TB_R_VOL,
    TB_L_BRI,
    TB_R_BRI,
    TB_L_ZOOM,
    TB_R_ZOOM,
    TB_L_ROT,
    TB_R_ROT,
};

enum status_error_code {
    STATUS_ERROR_NONE,
    STATUS_ERROR_SPLIT,
    STATUS_ERROR_POINTING,
};

enum trackball_side {
    TB_SIDE_LEFT,
    TB_SIDE_RIGHT,
};

enum trackball_mode {
    TB_MODE_CURSOR,
    TB_MODE_SMART_SCROLL,
    TB_MODE_VSCROLL,
    TB_MODE_HSCROLL,
    TB_MODE_PAN,
    TB_MODE_VOLUME,
    TB_MODE_BRIGHTNESS,
    TB_MODE_ZOOM,
    TB_MODE_ROTATE,
};

enum trackball_axis_lock {
    TB_AXIS_NONE,
    TB_AXIS_VERTICAL,
    TB_AXIS_HORIZONTAL,
};

typedef struct {
    enum trackball_mode      held_mode;
    enum trackball_mode      latched_mode;
    enum trackball_axis_lock axis_lock;
    uint32_t                 key_timer;
    uint32_t                 activity_timer;
    int16_t                  v_accum;
    int16_t                  h_accum;
    int16_t                  key_accum;
    bool                     moved_while_held;
} trackball_state_t;

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
#define TB_MODE_TIMEOUT_MS 1200
#define TB_AXIS_TIMEOUT_MS 600
#define TB_AXIS_LOCK_THRESHOLD 4
#define TB_AXIS_LOCK_RATIO 2
#define TB_SCROLL_DIVISOR 8
#define TB_KEY_THRESHOLD 24
#define TB_ROTATE_THRESHOLD 32

static bool     status_enabled          = true;
static uint8_t  status_brightness       = 64;
static uint32_t status_startup_timer    = 0;
static uint32_t status_error_timer      = 0;
static uint32_t status_error_check_timer = 0;
static uint8_t  status_error_code       = STATUS_ERROR_NONE;

static status_hsv_t status_last_hsv = {HSV_OFF};

static trackball_state_t trackball_states[] = {
    [TB_SIDE_LEFT]  = {0},
    [TB_SIDE_RIGHT] = {0},
};

static void status_render(void);

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
        KC_TRNS  , KC_7     , KC_5     , KC_3     , KC_1     , KC_9     , TB_BASE     ,                       TB_BASE , KC_0     , KC_2     , KC_4     , KC_6     , KC_8     , KC_TRNS,
        KC_TRNS  , KC_LT    , KC_GT    , KC_DLR   , KC_AMPR  , KC_MINS  , KC_TRNS  ,                       KC_NO    , KC_SLSH  , KC_BSLS  , KC_AT    , KC_HASH  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_LBRC  , KC_LCBR  , KC_RCBR  , KC_LPRN  , KC_EQL   , KC_NO    ,                       KC_NO    , KC_ASTR  , KC_RPRN  , KC_PLUS  , KC_RBRC  , KC_EXLM  , KC_TRNS,
        KC_TRNS  , KC_QUOT  , KC_NO    , KC_NO    , KC_TILD  , KC_PERC  , KC_NO    ,                       KC_NO    , KC_UNDS  , KC_QUES  , KC_PIPE  , KC_CIRC  , KC_GRV   , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_MOU] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TB_BASE ,                       TB_BASE , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS  ,                       KC_NO    , CPI_UP   , MBCK     , KC_NO    , MFWD     , MS_WHLU  , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       KC_NO    , CPI_DOWN , LCLK     , RCLK     , MCLK     , MS_WHLD  , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       SCRL_MOD , CPI_DFLT , TB_L_ZOOM_KEY, TB_L_ROT_KEY, TB_R_SMRT_KEY, TB_R_PAN_KEY, KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_NAV] = LAYOUT(
        KC_TRNS   , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , TB_BASE      ,                         TB_BASE  , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_TRNS,
        KC_CAPS   , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_TRNS   ,                         KC_NO     , KC_PGUP   , MBCK      , KC_UP     , MFWD      , KC_NO     , KC_TRNS,
        KC_TRNS   , KC_LSFT   , KC_LCTL   , KC_LALT   , KC_LGUI   , KC_NO     , KC_NO     ,                         KC_NO     , KC_PGDN   , KC_LEFT   , KC_DOWN   , KC_RGHT   , KC_NO     , KC_TRNS,
        KC_TRNS   , KC_NO     , LGUI(KC_X), LGUI(KC_C), LGUI(KC_V), KC_NO     , KC_NO     ,                         KC_NO     , KC_NO     , KC_HOME   , KC_NO     , KC_END    , KC_NO     , KC_TRNS,
                                KC_TRNS   , KC_TRNS   , KC_TRNS   ,                                                                         KC_TRNS   , KC_TRNS   , KC_TRNS,
                                                        KC_TRNS   ,                                                                         KC_TRNS,
                                                                    KC_TRNS   ,                                                 KC_TRNS,
                                                                                KC_TRNS   ,                         KC_TRNS
    ),

    [_NUM] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TB_BASE     ,                       TB_BASE , KC_NUM   , KC_PSLS  , KC_PAST  , KC_PMNS  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS  ,                       KC_NO    , KC_P7    , KC_P8    , KC_P9    , KC_PPLS  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_LSFT  , KC_LCTL  , KC_LALT  , KC_LGUI  , KC_NO    , KC_NO    ,                       KC_NO    , KC_P4    , KC_P5    , KC_P6    , KC_PENT  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       KC_NO    , KC_P1    , KC_P2    , KC_P3    , KC_PDOT  , KC_NO    , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_P0    , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_FN1] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TB_BASE     ,                       TB_BASE , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_F1    , KC_F2    , KC_F3    , KC_F4    , KC_NO    , KC_TRNS  ,                       KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_F5    , KC_F6    , KC_F7    , KC_F8    , KC_NO    , KC_NO    ,                       TT(_FN2) , KC_NO    , KC_RGUI  , KC_RALT  , KC_RCTL  , KC_RSFT  , KC_TRNS,
        KC_TRNS  , KC_F9    , KC_F10   , KC_F11   , KC_F12   , KC_NO    , KC_NO    ,                       KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             TT(_FN2),
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_FN2] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TB_BASE     ,                       TB_BASE , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_F13   , KC_F14   , KC_F15   , KC_F16   , KC_NO    , KC_TRNS  ,                       KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_F17   , KC_F18   , KC_F19   , KC_F20   , KC_NO    , KC_NO    ,                       KC_NO    , KC_NO    , KC_RGUI  , KC_RALT  , KC_RCTL  , KC_RSFT  , KC_TRNS,
        KC_TRNS  , KC_F21   , KC_F22   , KC_F23   , KC_F24   , KC_NO    , KC_NO    ,                       KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
                              KC_TRNS  , KC_TRNS  , KC_TRNS  ,                                                                   KC_TRNS  , KC_TRNS  , KC_TRNS,
                                                    KC_TRNS  ,                                                                   KC_TRNS,
                                                               KC_TRNS  ,                                             KC_TRNS,
                                                                          KC_TRNS  ,                       KC_TRNS
    ),

    [_SYS] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TB_BASE     ,                       TB_BASE , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
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

#ifdef POINTING_DEVICE_ENABLE
#    define LEFT_POINTING_ROTATION_SCALE 181
#    define LEFT_POINTING_ROTATION_SHIFT 8

static mouse_xy_report_t status_clamp_mouse_xy(int32_t value) {
    if (value < MOUSE_REPORT_XY_MIN) {
        return MOUSE_REPORT_XY_MIN;
    } else if (value > MOUSE_REPORT_XY_MAX) {
        return MOUSE_REPORT_XY_MAX;
    }

    return value;
}

static report_mouse_t rotate_left_pointing_report(report_mouse_t report) {
    int32_t x = report.x;
    int32_t y = report.y;

    report.x = status_clamp_mouse_xy(((x - y) * LEFT_POINTING_ROTATION_SCALE) >> LEFT_POINTING_ROTATION_SHIFT);
    report.y = status_clamp_mouse_xy(((x + y) * LEFT_POINTING_ROTATION_SCALE) >> LEFT_POINTING_ROTATION_SHIFT);

    return report;
}

static trackball_state_t *trackball_state_for_side(enum trackball_side side) {
    return &trackball_states[side];
}

static enum trackball_mode trackball_effective_mode(enum trackball_side side) __attribute__((unused));
static enum trackball_mode trackball_effective_mode(enum trackball_side side) {
    trackball_state_t *state = trackball_state_for_side(side);

    if (state->held_mode != TB_MODE_CURSOR) {
        return state->held_mode;
    }

    return state->latched_mode;
}

static void trackball_clear_side(enum trackball_side side) {
    trackball_state_t *state = trackball_state_for_side(side);

    state->held_mode        = TB_MODE_CURSOR;
    state->latched_mode     = TB_MODE_CURSOR;
    state->axis_lock        = TB_AXIS_NONE;
    state->key_timer        = 0;
    state->activity_timer   = 0;
    state->v_accum          = 0;
    state->h_accum          = 0;
    state->key_accum        = 0;
    state->moved_while_held = false;
}

static void trackball_clear_all(void) {
    trackball_clear_side(TB_SIDE_LEFT);
    trackball_clear_side(TB_SIDE_RIGHT);
}

static void reset_keyboard_state_to_base(void) {
    trackball_clear_all();
    layer_clear();
    status_render();
}

static int16_t trackball_abs16(int16_t value) {
    return value < 0 ? -value : value;
}

static bool trackball_report_has_motion(report_mouse_t report) {
    return report.x != 0 || report.y != 0;
}

static void trackball_mark_activity(enum trackball_side side) {
    trackball_state_t *state = trackball_state_for_side(side);

    state->activity_timer = timer_read32();

    if (state->held_mode != TB_MODE_CURSOR) {
        state->moved_while_held = true;
    }
}

static enum trackball_axis_lock trackball_choose_axis(report_mouse_t report) {
    int16_t abs_x = trackball_abs16(report.x);
    int16_t abs_y = trackball_abs16(report.y);

    if (abs_y >= TB_AXIS_LOCK_THRESHOLD && abs_y >= abs_x * TB_AXIS_LOCK_RATIO) {
        return TB_AXIS_VERTICAL;
    }

    if (abs_x >= TB_AXIS_LOCK_THRESHOLD && abs_x >= abs_y * TB_AXIS_LOCK_RATIO) {
        return TB_AXIS_HORIZONTAL;
    }

    return TB_AXIS_NONE;
}

static mouse_hv_report_t trackball_scroll_value(int16_t *accum, int16_t delta) {
    mouse_hv_report_t output = 0;

    *accum += delta;

    while (*accum >= TB_SCROLL_DIVISOR) {
        output++;
        *accum -= TB_SCROLL_DIVISOR;
    }

    while (*accum <= -TB_SCROLL_DIVISOR) {
        output--;
        *accum += TB_SCROLL_DIVISOR;
    }

    return output;
}

static void trackball_tap_by_accum(int16_t *accum, int16_t delta, uint16_t positive_keycode, uint16_t negative_keycode) {
    *accum += delta;

    while (*accum >= TB_KEY_THRESHOLD) {
        tap_code16(positive_keycode);
        *accum -= TB_KEY_THRESHOLD;
    }

    while (*accum <= -TB_KEY_THRESHOLD) {
        tap_code16(negative_keycode);
        *accum += TB_KEY_THRESHOLD;
    }
}

static void trackball_tap_rotate(int16_t *accum, int16_t delta) {
    *accum += delta;

    while (*accum >= TB_ROTATE_THRESHOLD) {
        tap_code16(LALT(KC_RBRC));
        *accum -= TB_ROTATE_THRESHOLD;
    }

    while (*accum <= -TB_ROTATE_THRESHOLD) {
        tap_code16(LALT(KC_LBRC));
        *accum += TB_ROTATE_THRESHOLD;
    }
}

static report_mouse_t trackball_process_side(enum trackball_side side, report_mouse_t report) {
    enum trackball_mode mode  = trackball_effective_mode(side);
    trackball_state_t  *state = trackball_state_for_side(side);

    if (mode == TB_MODE_CURSOR || !trackball_report_has_motion(report)) {
        return report;
    }

    trackball_mark_activity(side);

    switch (mode) {
        case TB_MODE_SMART_SCROLL:
            if (state->axis_lock == TB_AXIS_NONE) {
                state->axis_lock = trackball_choose_axis(report);
            }

            if (state->axis_lock == TB_AXIS_VERTICAL) {
                report.v = trackball_scroll_value(&state->v_accum, -report.y);
                report.x = 0;
                report.y = 0;
            } else if (state->axis_lock == TB_AXIS_HORIZONTAL) {
                report.h = trackball_scroll_value(&state->h_accum, report.x);
                report.x = 0;
                report.y = 0;
            } else {
                report.x = 0;
                report.y = 0;
            }
            break;

        case TB_MODE_VSCROLL:
            report.v = trackball_scroll_value(&state->v_accum, -report.y);
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_HSCROLL:
            report.h = trackball_scroll_value(&state->h_accum, report.x);
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_PAN:
            report.h = trackball_scroll_value(&state->h_accum, report.x);
            report.v = trackball_scroll_value(&state->v_accum, -report.y);
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_VOLUME:
            trackball_tap_by_accum(&state->key_accum, -report.y, KC_VOLU, KC_VOLD);
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_BRIGHTNESS:
            trackball_tap_by_accum(&state->key_accum, -report.y, KC_BRIU, KC_BRID);
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_ZOOM:
            trackball_tap_by_accum(&state->key_accum, -report.y, LGUI(KC_EQUAL), LGUI(KC_MINUS));
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_ROTATE:
            trackball_tap_rotate(&state->key_accum, report.x);
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_CURSOR:
            break;
    }

    return report;
}

report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
    left_report  = rotate_left_pointing_report(left_report);
    left_report  = trackball_process_side(TB_SIDE_LEFT, left_report);
    right_report = trackball_process_side(TB_SIDE_RIGHT, right_report);

    return pointing_device_combine_reports(left_report, right_report);
}
#endif

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
    trackball_clear_all();
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

static void trackball_timeout_task(void) {
    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        trackball_state_t *state = &trackball_states[side];

        if (state->latched_mode != TB_MODE_CURSOR && state->activity_timer != 0 && timer_elapsed32(state->activity_timer) > TB_MODE_TIMEOUT_MS) {
            trackball_clear_side(side);
        } else if (state->axis_lock != TB_AXIS_NONE && state->activity_timer != 0 && timer_elapsed32(state->activity_timer) > TB_AXIS_TIMEOUT_MS) {
            state->axis_lock = TB_AXIS_NONE;
            state->v_accum   = 0;
            state->h_accum   = 0;
            state->key_accum = 0;
        }
    }
}

void housekeeping_task_user(void) {
    trackball_timeout_task();
    status_render();
}

static bool trackball_keycode_to_mode(uint16_t keycode, enum trackball_side *side, enum trackball_mode *mode) {
    switch (keycode) {
        case TB_L_SMRT:
            *side = TB_SIDE_LEFT;
            *mode = TB_MODE_SMART_SCROLL;
            return true;
        case TB_R_SMRT:
            *side = TB_SIDE_RIGHT;
            *mode = TB_MODE_SMART_SCROLL;
            return true;
        case TB_L_VSCR:
            *side = TB_SIDE_LEFT;
            *mode = TB_MODE_VSCROLL;
            return true;
        case TB_R_VSCR:
            *side = TB_SIDE_RIGHT;
            *mode = TB_MODE_VSCROLL;
            return true;
        case TB_L_HSCR:
            *side = TB_SIDE_LEFT;
            *mode = TB_MODE_HSCROLL;
            return true;
        case TB_R_HSCR:
            *side = TB_SIDE_RIGHT;
            *mode = TB_MODE_HSCROLL;
            return true;
        case TB_L_PAN:
            *side = TB_SIDE_LEFT;
            *mode = TB_MODE_PAN;
            return true;
        case TB_R_PAN:
            *side = TB_SIDE_RIGHT;
            *mode = TB_MODE_PAN;
            return true;
        case TB_L_VOL:
            *side = TB_SIDE_LEFT;
            *mode = TB_MODE_VOLUME;
            return true;
        case TB_R_VOL:
            *side = TB_SIDE_RIGHT;
            *mode = TB_MODE_VOLUME;
            return true;
        case TB_L_BRI:
            *side = TB_SIDE_LEFT;
            *mode = TB_MODE_BRIGHTNESS;
            return true;
        case TB_R_BRI:
            *side = TB_SIDE_RIGHT;
            *mode = TB_MODE_BRIGHTNESS;
            return true;
        case TB_L_ZOOM:
            *side = TB_SIDE_LEFT;
            *mode = TB_MODE_ZOOM;
            return true;
        case TB_R_ZOOM:
            *side = TB_SIDE_RIGHT;
            *mode = TB_MODE_ZOOM;
            return true;
        case TB_L_ROT:
            *side = TB_SIDE_LEFT;
            *mode = TB_MODE_ROTATE;
            return true;
        case TB_R_ROT:
            *side = TB_SIDE_RIGHT;
            *mode = TB_MODE_ROTATE;
            return true;
    }

    return false;
}

static void trackball_mode_key_pressed(enum trackball_side side, enum trackball_mode mode) {
    trackball_state_t *state = trackball_state_for_side(side);

    state->held_mode        = mode;
    state->key_timer        = timer_read32();
    state->activity_timer   = state->key_timer;
    state->axis_lock        = TB_AXIS_NONE;
    state->moved_while_held = false;
    state->v_accum          = 0;
    state->h_accum          = 0;
    state->key_accum        = 0;
}

static void trackball_mode_key_released(enum trackball_side side) {
    trackball_state_t *state = trackball_state_for_side(side);

    if (!state->moved_while_held && timer_elapsed32(state->key_timer) <= TAPPING_TERM && state->held_mode != TB_MODE_CURSOR) {
        state->latched_mode   = state->held_mode;
        state->activity_timer = timer_read32();
    }

    state->held_mode        = TB_MODE_CURSOR;
    state->axis_lock        = TB_AXIS_NONE;
    state->moved_while_held = false;
    state->v_accum          = 0;
    state->h_accum          = 0;
    state->key_accum        = 0;
}

bool process_record_user(uint16_t keycode, keyrecord_t *record) {
    enum trackball_side side;
    enum trackball_mode mode;

    if (trackball_keycode_to_mode(keycode, &side, &mode)) {
        if (record->event.pressed) {
            trackball_mode_key_pressed(side, mode);
        } else {
            trackball_mode_key_released(side);
        }

        status_render();
        return false;
    }

    if (!record->event.pressed) {
        return true;
    }

    switch (keycode) {
        case TB_BASE:
            reset_keyboard_state_to_base();
            return false;

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
