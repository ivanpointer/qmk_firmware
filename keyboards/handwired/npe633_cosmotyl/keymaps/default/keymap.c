#include QMK_KEYBOARD_H
#include "split_util.h"
#include "transactions.h"
#include "usb_device_state.h"

#define LCLK MS_BTN1
#define RCLK MS_BTN2
#define MCLK MS_BTN3
#define MBCK MS_BTN4
#define MFWD MS_BTN5

// Trackball mode aliases used in the layout below.
#define SCRL_MOD TB_L_SMRT
#define SCRL_UP  TB_SCRL_UP
#define SCRL_DOWN TB_SCRL_DOWN
#define SCRL_DFLT TB_SCRL_DFLT
#define CPI_UP   TB_CPI_UP
#define CPI_DOWN TB_CPI_DOWN
#define CPI_DFLT TB_CPI_DFLT
#define PREC_MOD TB_PREC
#define BOOT_MODE STAT_BOOT
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
    STAT_BOOT,

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
    TB_SCRL_UP,
    TB_SCRL_DOWN,
    TB_SCRL_DFLT,
    TB_CPI_UP,
    TB_CPI_DOWN,
    TB_CPI_DFLT,
    TB_PREC,
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

enum status_light {
    STATUS_LIGHT_POINTER,
    STATUS_LIGHT_MOUSE,
    STATUS_LIGHT_LAYER,
    STATUS_LIGHT_BOARD,
    STATUS_LIGHT_COUNT,
};

enum status_level_overlay_kind {
    STATUS_LEVEL_NONE,
    STATUS_LEVEL_SCROLL,
    STATUS_LEVEL_CPI,
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
    uint8_t      brightness_percent;
} status_layer_style_t;

typedef struct {
    status_hsv_t leds[STATUS_LIGHT_COUNT];
} status_frame_t;

typedef struct {
    uint8_t             error_code;
    uint8_t             active_layer;
    bool                startup_active;
    bool                suspended;
    bool                host_connected;
    bool                caps_lock;
    bool                num_lock;
    bool                scroll_lock;
    bool                precision_active;
    uint8_t             cpi_level;
    enum trackball_mode left_mode;
    enum trackball_mode right_mode;
} status_model_t;

#define STATUS_BRIGHTNESS_STEP 16
#define STATUS_BRIGHTNESS_MIN 16
#define STATUS_BRIGHTNESS_MAX 96
#define STATUS_STARTUP_MS 1200
#define STATUS_STARTUP_STEP_MS 150
#define STATUS_BOOT_SIGNAL_MS 120
#define STATUS_SLEEP_BREATHE_MS 3000
#define STATUS_SLEEP_BREATHE_MIN 4
#define STATUS_NO_HOST_BREATHE_MS 1800
#define STATUS_NO_HOST_BREATHE_MIN 4
#define STATUS_LEVEL_OVERLAY_MS 900
#define STATUS_FRAME_ANIMATION_MS 50
#define STATUS_FRAME_REFRESH_MS 1000
#define STATUS_ERROR_ON_MS 160
#define STATUS_ERROR_OFF_MS 160
#define STATUS_ERROR_PAUSE_MS 900
#define STATUS_ERROR_CHECK_MS 250
#define STATUS_BREATHE_MS 2000
#define STATUS_LAYER_ALTERNATE_MS 700
#define STATUS_BREATHE_MIN 8
#define STATUS_LAYER_BRIGHTNESS_FULL 100
#define STATUS_LAYER_BRIGHTNESS_MID 80
#define STATUS_LAYER_BRIGHTNESS_DIM 60
#define TB_MODE_TIMEOUT_MS 1200
#define TB_AXIS_TIMEOUT_MS 600
#define TB_AXIS_LOCK_THRESHOLD 4
#define TB_AXIS_LOCK_RATIO 2
// Higher values make trackball scrolling and panning less sensitive.
#define TB_SCROLL_DEFAULT_DIVISOR 64
#define TB_SCROLL_DEFAULT_LEVEL 3
#define TB_CPI_DEFAULT 2000
#define TB_CPI_DEFAULT_LEVEL 3
#define TB_CPI_PRECISION 500
#define TB_KEY_THRESHOLD 24
#define TB_ROTATE_THRESHOLD 32

static bool     status_enabled          = true;
static uint8_t  status_brightness       = 64;
static uint32_t status_startup_timer    = 0;
static uint32_t status_error_timer      = 0;
static uint32_t status_error_check_timer = 0;
static uint8_t  status_error_code       = STATUS_ERROR_NONE;
static bool     status_initialized      = false;
static bool     status_suspended        = false;
static bool     status_caps_lock        = false;
static bool     status_num_lock         = false;
static bool     status_scroll_lock      = false;
static uint32_t status_level_overlay_timer = 0;
static uint8_t  status_level_overlay_level = 0;
static enum status_level_overlay_kind status_level_overlay_kind = STATUS_LEVEL_NONE;

static status_frame_t status_last_local_frame = {0};
static status_frame_t status_last_remote_frame = {0};
static bool           status_last_local_frame_valid = false;
static bool           status_last_remote_frame_valid = false;
static uint32_t       status_local_refresh_timer = 0;
static uint32_t       status_remote_refresh_timer = 0;

static trackball_state_t trackball_states[] = {
    [TB_SIDE_LEFT]  = {0},
    [TB_SIDE_RIGHT] = {0},
};

static const uint8_t  trackball_scroll_divisors[] = {192, 128, 96, TB_SCROLL_DEFAULT_DIVISOR, 40, 24, 12};
static uint8_t        trackball_scroll_level      = TB_SCROLL_DEFAULT_LEVEL;
static const uint16_t trackball_cpi_levels[] = {500, 750, 1000, TB_CPI_DEFAULT, 3000, 5000, 8000};
static uint8_t        trackball_cpi_level    = TB_CPI_DEFAULT_LEVEL;
static bool           trackball_precision_active = false;

static void status_render(void);
static void status_show_level_overlay(enum status_level_overlay_kind kind, uint8_t level);

static const status_layer_style_t status_layer_styles[] = {
    [_BASE] = {{HSV_WHITE}, {HSV_WHITE}, false, STATUS_LAYER_BRIGHTNESS_DIM},
    [_SH2]  = {{HSV_WHITE}, {HSV_BLUE}, true, STATUS_LAYER_BRIGHTNESS_FULL},
    [_MOU]  = {{HSV_YELLOW}, {HSV_YELLOW}, false, STATUS_LAYER_BRIGHTNESS_FULL},
    [_NAV]  = {{HSV_GREEN}, {HSV_GREEN}, false, STATUS_LAYER_BRIGHTNESS_FULL},
    [_NUM]  = {{HSV_BLUE}, {HSV_BLUE}, false, STATUS_LAYER_BRIGHTNESS_FULL},
    [_FN1]  = {{HSV_PURPLE}, {HSV_PURPLE}, false, STATUS_LAYER_BRIGHTNESS_FULL},
    [_FN2]  = {{HSV_PURPLE}, {HSV_CYAN}, true, STATUS_LAYER_BRIGHTNESS_FULL},
    [_SYS]  = {{HSV_PURPLE}, {HSV_YELLOW}, true, STATUS_LAYER_BRIGHTNESS_FULL},
};

static const status_hsv_t status_level_colors[] = {
    {HSV_BLUE},
    {150, 170, 255},
    {HSV_CYAN},
    {HSV_GREEN},
    {HSV_YELLOW},
    {32, 255, 255},
    {HSV_ORANGE},
};

static const status_hsv_t status_caps_lock_color   = {HSV_GREEN};
static const status_hsv_t status_num_lock_color    = {HSV_ORANGE};
static const status_hsv_t status_scroll_lock_color = {HSV_CYAN};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
    [_BASE] = LAYOUT(
        KC_ESC  , KC_LBRC , KC_LCBR , KC_RCBR , KC_LPRN , KC_EQUAL, PREC_MOD,                     KC_NO   , KC_ASTR , KC_RPRN , KC_PLUS , KC_RBRC , KC_EXLM , KC_BSPC,
        MO(_SH2), KC_SCLN , KC_COMM , KC_DOT  , KC_P    , KC_Y    , KC_TAB  ,                     KC_NO   , KC_F    , KC_G    , KC_C    , KC_R    , KC_L    , MO(_SH2),
        KC_LSFT , KC_A    , KC_O    , KC_E    , KC_U    , KC_I    , TT(_NUM),                     TT(_FN1), KC_D    , KC_H    , KC_T    , KC_N    , KC_S    , KC_RSFT,
        KC_LCTL , KC_QUOT , KC_Q    , KC_J    , KC_K    , KC_X    , SCRL_MOD,                     TT(_SYS), KC_B    , KC_M    , KC_W    , KC_V    , KC_Z    , KC_RCTL,
                            KC_LALT , KC_LGUI , LCLK    ,                                                             TT(_MOU), KC_RGUI , KC_RALT,
                                                KC_ENT  ,                                                             KC_SPC,
                                                          RCLK    ,                                         TT(_FN1),
                                                                    MCLK    ,                     TT(_NAV)
    ),

    [_SH2] = LAYOUT(
        KC_TRNS , KC_7    , KC_5    , KC_3    , KC_1    , KC_9    , TB_BASE ,                     TB_BASE , KC_0    , KC_2    , KC_4    , KC_6    , KC_8    , KC_TRNS,
        KC_TRNS , KC_LT   , KC_GT   , KC_DLR  , KC_AMPR , KC_MINS , KC_TRNS ,                     KC_NO   , KC_MINS , KC_SLSH , KC_BSLS , KC_AT   , KC_HASH , KC_TRNS,
        KC_TRNS , KC_LBRC , KC_LCBR , KC_RCBR , KC_LPRN , KC_EQL  , KC_NO   ,                     KC_NO   , KC_ASTR , KC_RPRN , KC_PLUS , KC_RBRC , KC_EXLM , KC_TRNS,
        KC_TRNS , KC_QUOT , KC_NO   , KC_NO   , KC_TILD , KC_PERC , SCRL_MOD,                     KC_NO   , KC_UNDS , KC_QUES , KC_PIPE , KC_CIRC , KC_GRV  , KC_TRNS,
                            KC_TRNS , KC_TRNS , KC_TRNS ,                                                             KC_TRNS , KC_TRNS , KC_TRNS,
                                                KC_TRNS ,                                                             KC_TRNS,
                                                          KC_TRNS ,                                         KC_TRNS,
                                                                    KC_TRNS ,                     KC_TRNS
    ),

    [_MOU] = LAYOUT(
        KC_TRNS      , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        , TB_BASE      ,                               TB_BASE      , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_TRNS,
        KC_TRNS      , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_TRNS      ,                               SCRL_UP      , CPI_UP       , MBCK         , KC_NO        , MFWD         , TB_R_SMRT_KEY, KC_TRNS,
        KC_TRNS      , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        ,                               SCRL_DOWN    , CPI_DOWN     , LCLK         , RCLK         , MCLK         , TB_R_SMRT_KEY, KC_TRNS,
        KC_TRNS      , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        , SCRL_MOD     ,                               SCRL_DFLT    , CPI_DFLT     , TB_L_ZOOM_KEY, TB_L_ROT_KEY , TB_R_SMRT_KEY, TB_R_PAN_KEY , KC_TRNS,
                                      KC_TRNS      , KC_TRNS      , KC_TRNS      ,                                                                                           KC_TRNS      , KC_TRNS      , KC_TRNS,
                                                                    KC_TRNS      ,                                                                                           KC_TRNS,
                                                                                   KC_TRNS      ,                                                             KC_TRNS,
                                                                                                  KC_TRNS      ,                               KC_TRNS
    ),

    [_NAV] = LAYOUT(
        KC_TRNS   , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , TB_BASE   ,                         TB_BASE   , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_TRNS,
        KC_CAPS   , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_NO     , KC_TRNS   ,                         KC_NO     , KC_PGUP   , MBCK      , KC_UP     , MFWD      , KC_NO     , KC_TRNS,
        KC_TRNS   , KC_LSFT   , KC_LCTL   , KC_LALT   , KC_LGUI   , KC_NO     , KC_NO     ,                         KC_NO     , KC_PGDN   , KC_LEFT   , KC_DOWN   , KC_RGHT   , KC_NO     , KC_TRNS,
        KC_TRNS   , KC_NO     , LGUI(KC_X), LGUI(KC_C), LGUI(KC_V), KC_NO     , SCRL_MOD  ,                         KC_NO     , KC_NO     , KC_HOME   , KC_NO     , KC_END    , KC_NO     , KC_TRNS,
                                KC_TRNS   , KC_TRNS   , KC_TRNS   ,                                                                         KC_TRNS   , KC_TRNS   , KC_TRNS,
                                                        KC_TRNS   ,                                                                         KC_TRNS,
                                                                    KC_TRNS   ,                                                 KC_TRNS,
                                                                                KC_TRNS   ,                         KC_TRNS
    ),

    [_NUM] = LAYOUT(
        KC_TRNS, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , TB_BASE,                   TB_BASE, KC_NUM , KC_PSLS, KC_PAST, KC_PMNS, KC_NO  , KC_TRNS,
        KC_TRNS, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_TRNS,                   KC_NO  , KC_P7  , KC_P8  , KC_P9  , KC_PPLS, KC_NO  , KC_TRNS,
        KC_TRNS, KC_LSFT, KC_LCTL, KC_LALT, KC_LGUI, KC_NO  , KC_NO  ,                   KC_NO  , KC_P4  , KC_P5  , KC_P6  , KC_PENT, KC_NO  , KC_TRNS,
        KC_TRNS, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,                   KC_NO  , KC_P1  , KC_P2  , KC_P3  , KC_PDOT, KC_NO  , KC_TRNS,
                          KC_TRNS, KC_TRNS, KC_TRNS,                                                       KC_TRNS, KC_P0  , KC_TRNS,
                                            KC_TRNS,                                                       KC_TRNS,
                                                     KC_TRNS,                                     KC_TRNS,
                                                              KC_TRNS,                   KC_TRNS
    ),

    [_FN1] = LAYOUT(
        KC_TRNS , KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_NO   , TB_BASE ,                     TB_BASE , KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_TRNS,
        KC_TRNS , KC_F1   , KC_F2   , KC_F3   , KC_F4   , KC_NO   , KC_TRNS ,                     KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_TRNS,
        KC_TRNS , KC_F5   , KC_F6   , KC_F7   , KC_F8   , KC_NO   , KC_NO   ,                     TT(_FN2), KC_NO   , KC_RGUI , KC_RALT , KC_RCTL , KC_RSFT , KC_TRNS,
        KC_TRNS , KC_F9   , KC_F10  , KC_F11  , KC_F12  , KC_NO   , KC_NO   ,                     KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_NO   , KC_TRNS,
                            KC_TRNS , KC_TRNS , KC_TRNS ,                                                             KC_TRNS , KC_TRNS , KC_TRNS,
                                                KC_TRNS ,                                                             KC_TRNS,
                                                          KC_TRNS ,                                         TT(_FN2),
                                                                    KC_TRNS ,                     KC_TRNS
    ),

    [_FN2] = LAYOUT(
        KC_TRNS, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , TB_BASE,                   TB_BASE, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_TRNS,
        KC_TRNS, KC_F13 , KC_F14 , KC_F15 , KC_F16 , KC_NO  , KC_TRNS,                   KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_TRNS,
        KC_TRNS, KC_F17 , KC_F18 , KC_F19 , KC_F20 , KC_NO  , KC_NO  ,                   KC_NO  , KC_NO  , KC_RGUI, KC_RALT, KC_RCTL, KC_RSFT, KC_TRNS,
        KC_TRNS, KC_F21 , KC_F22 , KC_F23 , KC_F24 , KC_NO  , KC_NO  ,                   KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_TRNS,
                          KC_TRNS, KC_TRNS, KC_TRNS,                                                       KC_TRNS, KC_TRNS, KC_TRNS,
                                            KC_TRNS,                                                       KC_TRNS,
                                                     KC_TRNS,                                     KC_TRNS,
                                                              KC_TRNS,                   KC_TRNS
    ),

    [_SYS] = LAYOUT(
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , TB_BASE  ,                       TB_BASE  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS  ,                       KC_NO    , KC_VOLU  , KC_MPRV  , KC_MSTP  , KC_MNXT  , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_LSFT  , KC_LCTL  , KC_LALT  , KC_LGUI  , KC_NO    , KC_NO    ,                       KC_NO    , KC_VOLD  , KC_NO    , KC_MPLY  , KC_NO    , KC_NO    , KC_TRNS,
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       BOOT_MODE, KC_MUTE  , STAT_BRID, STAT_TEST, STAT_BRIU, STAT_TOGG, KC_TRNS,
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

static void trackball_clear_scroll_accums(void) {
    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        trackball_states[side].v_accum = 0;
        trackball_states[side].h_accum = 0;
    }
}

static uint8_t trackball_scroll_current_divisor(void) {
    return trackball_scroll_divisors[trackball_scroll_level];
}

static void trackball_scroll_speed_up(void) {
    if (trackball_scroll_level < ARRAY_SIZE(trackball_scroll_divisors) - 1) {
        trackball_scroll_level++;
        trackball_clear_scroll_accums();
    }

    status_show_level_overlay(STATUS_LEVEL_SCROLL, trackball_scroll_level);
}

static void trackball_scroll_speed_down(void) {
    if (trackball_scroll_level > 0) {
        trackball_scroll_level--;
        trackball_clear_scroll_accums();
    }

    status_show_level_overlay(STATUS_LEVEL_SCROLL, trackball_scroll_level);
}

static void trackball_scroll_speed_default(void) {
    trackball_scroll_level = TB_SCROLL_DEFAULT_LEVEL;
    trackball_clear_scroll_accums();
    status_show_level_overlay(STATUS_LEVEL_SCROLL, trackball_scroll_level);
}

static void trackball_set_cpi(uint16_t cpi) {
#if defined(SPLIT_POINTING_ENABLE) && defined(POINTING_DEVICE_COMBINED)
    pointing_device_set_cpi_on_side(true, cpi);
    pointing_device_set_cpi_on_side(false, cpi);
#else
    pointing_device_set_cpi(cpi);
#endif
}

static void trackball_apply_cpi(void) {
    trackball_set_cpi(trackball_cpi_levels[trackball_cpi_level]);
}

static void trackball_cpi_up(void) {
    if (trackball_cpi_level < ARRAY_SIZE(trackball_cpi_levels) - 1) {
        trackball_cpi_level++;
        if (!trackball_precision_active) {
            trackball_apply_cpi();
        }
    }

    status_show_level_overlay(STATUS_LEVEL_CPI, trackball_cpi_level);
}

static void trackball_cpi_down(void) {
    if (trackball_cpi_level > 0) {
        trackball_cpi_level--;
        if (!trackball_precision_active) {
            trackball_apply_cpi();
        }
    }

    status_show_level_overlay(STATUS_LEVEL_CPI, trackball_cpi_level);
}

static void trackball_cpi_default(void) {
    trackball_cpi_level = TB_CPI_DEFAULT_LEVEL;
    if (!trackball_precision_active) {
        trackball_apply_cpi();
    }

    status_show_level_overlay(STATUS_LEVEL_CPI, trackball_cpi_level);
}

static void trackball_precision_set(bool active) {
    trackball_precision_active = active;

    if (active) {
        trackball_set_cpi(TB_CPI_PRECISION);
        return;
    }

    trackball_apply_cpi();
}

static void reset_keyboard_state_to_base(void) {
    trackball_clear_all();
    trackball_precision_set(false);
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
    uint8_t           divisor = trackball_scroll_current_divisor();

    *accum += delta;

    while (*accum >= divisor) {
        output++;
        *accum -= divisor;
    }

    while (*accum <= -divisor) {
        output--;
        *accum += divisor;
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

static bool status_hsv_equal(status_hsv_t a, status_hsv_t b) {
    return a.h == b.h && a.s == b.s && a.v == b.v;
}

static bool status_frame_equal(status_frame_t a, status_frame_t b) {
    for (uint8_t i = 0; i < STATUS_LIGHT_COUNT; i++) {
        if (!status_hsv_equal(a.leds[i], b.leds[i])) {
            return false;
        }
    }

    return true;
}

static void status_fill_frame(status_frame_t *frame, status_hsv_t color) {
    for (uint8_t i = 0; i < STATUS_LIGHT_COUNT; i++) {
        frame->leds[i] = color;
    }
}

static uint8_t status_local_led_offset(void) {
#if defined(SPLIT_KEYBOARD) && defined(RGBLED_SPLIT)
    return is_keyboard_left() ? 0 : STATUS_LIGHT_COUNT;
#else
    return 0;
#endif
}

static bool status_side_is_local(enum trackball_side side) {
#ifdef SPLIT_KEYBOARD
    return (side == TB_SIDE_LEFT) == is_keyboard_left();
#else
    return side == TB_SIDE_LEFT;
#endif
}

static void status_apply_local_frame(status_frame_t frame) {
    uint32_t elapsed     = timer_elapsed32(status_local_refresh_timer);
    bool     same_frame  = status_last_local_frame_valid && status_frame_equal(frame, status_last_local_frame);
    bool     refresh_due = elapsed >= STATUS_FRAME_REFRESH_MS;

    if (status_last_local_frame_valid) {
        if (same_frame && !refresh_due) {
            return;
        }

        if (!same_frame && elapsed < STATUS_FRAME_ANIMATION_MS) {
            return;
        }
    }

    uint8_t offset = status_local_led_offset();
    for (uint8_t i = 0; i < STATUS_LIGHT_COUNT; i++) {
        status_hsv_t color = frame.leds[i];
        rgblight_sethsv_at(color.h, color.s, color.v, offset + i);
    }

    status_last_local_frame       = frame;
    status_last_local_frame_valid = true;
    status_local_refresh_timer    = timer_read32();
}

static void status_apply_remote_frame(status_frame_t frame) {
#if defined(SPLIT_KEYBOARD) && defined(SPLIT_TRANSACTION_RPC)
    if (!is_keyboard_master()) {
        return;
    }

    uint32_t elapsed     = timer_elapsed32(status_remote_refresh_timer);
    bool     same_frame  = status_last_remote_frame_valid && status_frame_equal(frame, status_last_remote_frame);
    bool     refresh_due = elapsed >= STATUS_FRAME_REFRESH_MS;

    if (status_last_remote_frame_valid) {
        if (same_frame && !refresh_due) {
            return;
        }

        if (!same_frame && elapsed < STATUS_FRAME_ANIMATION_MS) {
            return;
        }
    }

    if (transaction_rpc_send(RPC_ID_STATUS_FRAME, sizeof(frame), &frame)) {
        status_last_remote_frame       = frame;
        status_last_remote_frame_valid = true;
        status_remote_refresh_timer    = timer_read32();
    }
#else
    (void)frame;
#endif
}

#if defined(SPLIT_KEYBOARD) && defined(SPLIT_TRANSACTION_RPC)
static void status_frame_slave_handler(uint8_t initiator2target_buffer_size, const void *initiator2target_buffer, uint8_t target2initiator_buffer_size, void *target2initiator_buffer) {
    (void)target2initiator_buffer_size;
    (void)target2initiator_buffer;

    if (initiator2target_buffer_size != sizeof(status_frame_t)) {
        return;
    }

    status_apply_local_frame(*(const status_frame_t *)initiator2target_buffer);
}
#endif

static void status_apply_side_frame(enum trackball_side side, status_frame_t frame) {
    if (status_side_is_local(side)) {
        status_apply_local_frame(frame);
    } else {
        status_apply_remote_frame(frame);
    }
}

static void status_apply_both_frames(status_frame_t left_frame, status_frame_t right_frame) {
#ifdef SPLIT_KEYBOARD
    if (!is_keyboard_master()) {
        return;
    }
#endif

    status_apply_side_frame(TB_SIDE_LEFT, left_frame);
    status_apply_side_frame(TB_SIDE_RIGHT, right_frame);
}

static void status_apply_all(status_hsv_t color) {
    status_frame_t frame = {0};
    status_fill_frame(&frame, color);
    status_apply_both_frames(frame, frame);
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

static status_frame_t status_startup_frame(void) {
    uint16_t elapsed = timer_elapsed32(status_startup_timer);
    bool     on      = ((elapsed / STATUS_STARTUP_STEP_MS) % 2) == 0;
    status_frame_t frame = {0};

    status_fill_frame(&frame, on ? (status_hsv_t){HSV_WHITE} : (status_hsv_t){HSV_OFF});
    return frame;
}

static status_frame_t status_sleep_frame(uint32_t now) {
    status_frame_t frame = {0};
    status_hsv_t color = {HSV_BLUE};
    color.v = status_wave_value(now, STATUS_SLEEP_BREATHE_MS, STATUS_SLEEP_BREATHE_MIN);
    frame.leds[STATUS_LIGHT_BOARD] = color;
    return frame;
}

static status_frame_t status_no_host_frame(uint32_t now) {
    status_frame_t frame = {0};
    status_hsv_t color = {HSV_MAGENTA};
    color.v = status_wave_value(now, STATUS_NO_HOST_BREATHE_MS, STATUS_NO_HOST_BREATHE_MIN);
    frame.leds[STATUS_LIGHT_BOARD] = color;
    return frame;
}

static void status_signal_bootloader(void) {
    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);

    status_apply_all((status_hsv_t){HSV_RED});
    wait_ms(STATUS_BOOT_SIGNAL_MS);
    status_apply_all((status_hsv_t){HSV_WHITE});
    wait_ms(STATUS_BOOT_SIGNAL_MS);
    status_apply_all((status_hsv_t){HSV_RED});
    wait_ms(STATUS_BOOT_SIGNAL_MS);
}

static status_hsv_t status_error_color(void) {
    uint16_t blink_window = STATUS_ERROR_ON_MS + STATUS_ERROR_OFF_MS;
    uint16_t cycle_ms     = (status_error_code * blink_window) + STATUS_ERROR_PAUSE_MS;
    uint16_t elapsed      = timer_elapsed32(status_error_timer) % cycle_ms;
    bool     on           = false;

    if (elapsed < status_error_code * blink_window) {
        on = (elapsed % blink_window) < STATUS_ERROR_ON_MS;
    }

    return on ? (status_hsv_t){HSV_RED} : (status_hsv_t){HSV_OFF};
}

static status_model_t status_collect_model(uint32_t now) {
    uint8_t               layer     = get_highest_layer(layer_state | default_layer_state);
    usb_configure_state_t usb_state = usb_device_state_get_configure_state();

    if (layer >= ARRAY_SIZE(status_layer_styles)) {
        layer = _BASE;
    }

    return (status_model_t){
        .error_code       = status_error_code,
        .active_layer     = layer,
        .startup_active   = timer_elapsed32(status_startup_timer) < STATUS_STARTUP_MS,
        .suspended        = status_suspended || usb_state == USB_DEVICE_STATE_SUSPEND,
        .host_connected   = usb_state == USB_DEVICE_STATE_CONFIGURED,
        .caps_lock        = status_caps_lock,
        .num_lock         = status_num_lock,
        .scroll_lock      = status_scroll_lock,
        .precision_active = trackball_precision_active,
        .cpi_level        = trackball_cpi_level,
        .left_mode        = trackball_effective_mode(TB_SIDE_LEFT),
        .right_mode       = trackball_effective_mode(TB_SIDE_RIGHT),
    };
}

static status_hsv_t status_color_for_trackball_mode(enum trackball_mode mode) {
    switch (mode) {
        case TB_MODE_SMART_SCROLL:
        case TB_MODE_VSCROLL:
        case TB_MODE_HSCROLL:
        case TB_MODE_PAN:
            return (status_hsv_t){HSV_CYAN};
        case TB_MODE_VOLUME:
            return (status_hsv_t){HSV_GREEN};
        case TB_MODE_BRIGHTNESS:
            return (status_hsv_t){HSV_YELLOW};
        case TB_MODE_ZOOM:
            return (status_hsv_t){HSV_BLUE};
        case TB_MODE_ROTATE:
            return (status_hsv_t){HSV_PURPLE};
        case TB_MODE_CURSOR:
            return (status_hsv_t){HSV_OFF};
    }

    return (status_hsv_t){HSV_OFF};
}

static uint8_t status_scaled_brightness(uint8_t brightness, uint8_t percent) {
    if (brightness == 0 || percent == 0) {
        return 0;
    }

    uint8_t scaled = ((uint16_t)brightness * percent + 50) / 100;
    return scaled > 0 ? scaled : 1;
}

static status_hsv_t status_layer_color(uint32_t now, status_model_t model) {
    status_layer_style_t style = status_layer_styles[model.active_layer];
    status_hsv_t         color = style.primary;

    if (style.alternate && ((now / STATUS_LAYER_ALTERNATE_MS) % 2) == 1) {
        color = style.secondary;
    }

    color.v = status_scaled_brightness(status_brightness, style.brightness_percent);
    return color;
}

static status_hsv_t status_lock_color(uint32_t now, status_model_t model) {
    if (!model.caps_lock && !model.num_lock && !model.scroll_lock) {
        status_hsv_t color = {HSV_WHITE};
        color.v = status_brightness;
        return color;
    }

    status_hsv_t lock_colors[3] = {0};
    uint8_t      lock_count     = 0;

    if (model.caps_lock) {
        lock_colors[lock_count++] = status_caps_lock_color;
    }

    if (model.num_lock) {
        lock_colors[lock_count++] = status_num_lock_color;
    }

    if (model.scroll_lock) {
        lock_colors[lock_count++] = status_scroll_lock_color;
    }

    status_hsv_t color = lock_colors[(now / STATUS_LAYER_ALTERNATE_MS) % lock_count];
    color.v = status_brightness;
    return color;
}

static status_hsv_t status_mode_color_for_side(status_model_t model, enum trackball_side side) {
    enum trackball_mode mode  = side == TB_SIDE_LEFT ? model.left_mode : model.right_mode;
    status_hsv_t        color = status_color_for_trackball_mode(mode);

    if (color.v != 0) {
        color.v = status_brightness;
    } else {
        color = (status_hsv_t){HSV_WHITE};
        color.v = status_brightness;
    }

    return color;
}

static status_hsv_t status_pointer_color(uint32_t now, status_model_t model) {
    uint8_t level = model.cpi_level;
    if (level >= ARRAY_SIZE(status_level_colors)) {
        level = TB_CPI_DEFAULT_LEVEL;
    }

    status_hsv_t color = status_level_colors[level];
    color.v = status_brightness;

    if (model.precision_active) {
        color = (status_hsv_t){HSV_WHITE};
        color.v = status_wave_value(now, STATUS_BREATHE_MS, STATUS_BREATHE_MIN);
    }

    return color;
}

static bool status_error_display_active(void) {
    return status_error_code != STATUS_ERROR_NONE;
}

static bool status_level_overlay_active(uint32_t now) {
    if (status_level_overlay_kind == STATUS_LEVEL_NONE) {
        return false;
    }

    if (timer_elapsed32(status_level_overlay_timer) < STATUS_LEVEL_OVERLAY_MS) {
        return true;
    }

    status_level_overlay_kind = STATUS_LEVEL_NONE;
    return false;
}

static status_frame_t status_level_overlay_frame(void) {
    status_frame_t frame = {0};
    uint8_t        level = status_level_overlay_level;

    if (level >= STATUS_LIGHT_COUNT * 2 - 1) {
        level = STATUS_LIGHT_COUNT * 2 - 2;
    }

    status_hsv_t color = status_level_colors[level];
    color.v = status_brightness;

    if ((level % 2) == 0) {
        frame.leds[level / 2] = color;
    } else {
        frame.leds[level / 2]     = color;
        frame.leds[level / 2 + 1] = color;
    }

    return frame;
}

static void status_show_level_overlay(enum status_level_overlay_kind kind, uint8_t level) {
    status_level_overlay_kind  = kind;
    status_level_overlay_level = level;
    status_level_overlay_timer = timer_read32();
    status_render();
}

static status_frame_t status_build_side_frame(uint32_t now, status_model_t model, enum trackball_side side) {
    status_frame_t frame = {0};

    if (model.startup_active) {
        return status_startup_frame();
    }

    if (status_level_overlay_active(now)) {
        return status_level_overlay_frame();
    }

    if (model.suspended) {
        return status_sleep_frame(now);
    }

    if (!model.host_connected) {
        return status_no_host_frame(now);
    }

    frame.leds[STATUS_LIGHT_BOARD]   = status_error_display_active() ? status_error_color() : status_lock_color(now, model);
    frame.leds[STATUS_LIGHT_LAYER]   = status_layer_color(now, model);
    frame.leds[STATUS_LIGHT_MOUSE]   = status_mode_color_for_side(model, side);
    frame.leds[STATUS_LIGHT_POINTER] = status_pointer_color(now, model);

    return frame;
}

static void status_render(void) {
    if (!status_initialized) {
        return;
    }

    uint32_t now = timer_read32();

    if (!status_enabled) {
        status_apply_all((status_hsv_t){HSV_OFF});
        return;
    }

    status_model_t model = status_collect_model(now);

    if (!model.startup_active && !model.suspended && model.host_connected) {
        status_update_error_code(now);
        model.error_code = status_error_code;
    }

    status_frame_t left_frame  = status_build_side_frame(now, model, TB_SIDE_LEFT);
    status_frame_t right_frame = status_build_side_frame(now, model, TB_SIDE_RIGHT);
    status_apply_both_frames(left_frame, right_frame);
}

void keyboard_post_init_user(void) {
#if defined(SPLIT_KEYBOARD) && defined(SPLIT_TRANSACTION_RPC)
    transaction_register_rpc(RPC_ID_STATUS_FRAME, status_frame_slave_handler);
#endif

    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    status_startup_timer     = timer_read32();
    status_error_timer       = status_startup_timer;
    status_error_check_timer = status_startup_timer;
    status_last_local_frame_valid = false;
    status_last_remote_frame_valid = false;
    status_local_refresh_timer = status_startup_timer;
    status_remote_refresh_timer = status_startup_timer;
    status_initialized       = true;
    trackball_clear_all();
    status_render();
}

void notify_usb_device_state_change_user(struct usb_device_state usb_state) {
    status_suspended = usb_state.configure_state == USB_DEVICE_STATE_SUSPEND;

    if (status_initialized) {
        status_render();
    }
}

void suspend_power_down_user(void) {
    status_suspended = true;
    status_render();
}

void suspend_wakeup_init_user(void) {
    status_suspended = false;
    status_last_local_frame_valid = false;
    status_last_remote_frame_valid = false;
    status_local_refresh_timer = timer_read32();
    status_remote_refresh_timer = status_local_refresh_timer;
    status_render();
}

layer_state_t layer_state_set_user(layer_state_t state) {
    status_render();
    return state;
}

bool led_update_user(led_t led_state) {
    status_caps_lock   = led_state.caps_lock;
    status_num_lock    = led_state.num_lock;
    status_scroll_lock = led_state.scroll_lock;
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

    if (keycode == TB_PREC) {
        trackball_precision_set(record->event.pressed);
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

        case TB_SCRL_UP:
            trackball_scroll_speed_up();
            return false;

        case TB_SCRL_DOWN:
            trackball_scroll_speed_down();
            return false;

        case TB_SCRL_DFLT:
            trackball_scroll_speed_default();
            return false;

        case TB_CPI_UP:
            trackball_cpi_up();
            return false;

        case TB_CPI_DOWN:
            trackball_cpi_down();
            return false;

        case TB_CPI_DFLT:
            trackball_cpi_default();
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

        case STAT_BOOT:
            status_signal_bootloader();
            bootloader_jump();
            return false;
    }

    return true;
}
