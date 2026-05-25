#include QMK_KEYBOARD_H
#include "eeconfig.h"
#include "raw_hid.h"
#include "split_util.h"
#include "transactions.h"
#include "usb_device_state.h"
#ifdef PROTOCOL_CHIBIOS
#    include "usb_main.h"
#endif

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
#define GAME_TOG TG(_GAME)
#define TO_BASE  TO(_BASE)

enum layers {
    _BASE,
    _SH2,
    _MOU,
    _NAV,
    _NUM,
    _FN1,
    _FN2,
    _SYS,
    _GAME,
    _LAYER_COUNT
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
    TB_L_SCRL_UP,
    TB_L_SCRL_DOWN,
    TB_L_SCRL_DFLT,
    TB_R_SCRL_UP,
    TB_R_SCRL_DOWN,
    TB_R_SCRL_DFLT,
    TB_CPI_UP,
    TB_CPI_DOWN,
    TB_CPI_DFLT,
    TB_L_CPI_UP,
    TB_L_CPI_DOWN,
    TB_L_CPI_DFLT,
    TB_R_CPI_UP,
    TB_R_CPI_DOWN,
    TB_R_CPI_DFLT,
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

enum trackball_target {
    TB_TARGET_LEFT  = 1 << TB_SIDE_LEFT,
    TB_TARGET_RIGHT = 1 << TB_SIDE_RIGHT,
    TB_TARGET_BOTH  = TB_TARGET_LEFT | TB_TARGET_RIGHT,
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

typedef struct {
    uint8_t flags;
    uint8_t motion;
} companion_sensor_state_t;

typedef struct {
    uint8_t active_layer;
    uint8_t lock_flags;
    uint8_t left_mode;
    uint8_t right_mode;
    uint8_t trackball_mode_flags;
    uint8_t scroll_level;
    uint8_t scroll_divisor;
    uint8_t cpi_level;
    uint16_t cpi_value;
    uint8_t right_scroll_level;
    uint8_t right_scroll_divisor;
    uint8_t right_cpi_level;
    uint16_t right_cpi_value;
    bool precision_active;
    bool startup_active;
    bool suspended;
    bool host_connected;
    uint8_t error_code;
    bool transport_connected;
    uint8_t pointing_status;
    bool local_side_left;
    bool master;
    bool status_enabled;
    uint8_t status_brightness;
    uint8_t lift_flags;
    uint8_t lift_valid_flags;
    uint8_t left_motion;
    uint8_t right_motion;
} companion_state_t;

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
#define STATUS_USB_RECOVERY_STUCK_MS 8000
#define STATUS_USB_RECOVERY_RETRY_MS 20000
#define TB_MODE_TIMEOUT_MS 1200
#define TB_AXIS_TIMEOUT_MS 600
#define TB_AXIS_LOCK_THRESHOLD 4
#define TB_AXIS_LOCK_RATIO 2
// Higher values make trackball scrolling and panning less sensitive.
#define TB_SCROLL_DEFAULT_DIVISOR 64
#define TB_SCROLL_DEFAULT_LEVEL 3
#define TB_CPI_LEVEL_COUNT 7
#define TB_CPI_DEFAULT 2000
#define TB_CPI_DEFAULT_LEVEL 3
#define TB_CPI_PRECISION 500
#define TB_SCROLL_MODE_CPI TB_CPI_DEFAULT
#define TB_CPI_MIN 50
#define TB_CPI_MAX 16000
#define TB_CPI_STEP 50
#define TB_SCROLL_DIVISOR_MIN 1
#define TB_SCROLL_DIVISOR_MAX 255
#define TB_LEVEL_EEPROM_MAGIC 0x4C50454E
#define TB_LEVEL_EEPROM_VERSION 2
#define TB_KEY_THRESHOLD 24
#define TB_ROTATE_THRESHOLD 32
#define COMPANION_REPORT_SIZE 32
#define COMPANION_MAGIC_0 'N'
#define COMPANION_MAGIC_1 'P'
#define COMPANION_PROTOCOL_VERSION 1
#define COMPANION_PAYLOAD_OFFSET 8
#define COMPANION_PAYLOAD_SIZE (COMPANION_REPORT_SIZE - COMPANION_PAYLOAD_OFFSET)
#define COMPANION_REMOTE_POLL_MS 25

enum companion_message_type {
    COMPANION_MSG_EVENT        = 0x01,
    COMPANION_MSG_QUERY_STATUS = 0x80,
    COMPANION_MSG_QUERY_CLASS  = 0x81,
    COMPANION_MSG_QUERY_CONFIG = 0x82,
    COMPANION_MSG_SET_CONFIG   = 0x83,
};

enum companion_event_class {
    COMPANION_CLASS_IDENTITY,
    COMPANION_CLASS_LAYER,
    COMPANION_CLASS_LOCKS,
    COMPANION_CLASS_TRACKBALL,
    COMPANION_CLASS_LEVELS,
    COMPANION_CLASS_USB,
    COMPANION_CLASS_SPLIT,
    COMPANION_CLASS_LIFT,
    COMPANION_CLASS_STATUS,
    COMPANION_CLASS_LAYER_COLOR,
    COMPANION_CLASS_LEVEL_TABLE,
    COMPANION_CLASS_STATUS_COLOR,
    COMPANION_CLASS_LAYER_ALIAS,
    COMPANION_CLASS_COUNT,
};

#define COMPANION_STATUS_CLASS_FIRST COMPANION_CLASS_IDENTITY
#define COMPANION_STATUS_CLASS_LAST COMPANION_CLASS_STATUS
#define COMPANION_CONFIG_CLASS_FIRST COMPANION_CLASS_LAYER_COLOR
#define COMPANION_CONFIG_CLASS_LAST COMPANION_CLASS_LAYER_ALIAS

enum companion_report_flags {
    COMPANION_REPORT_LOCAL_LEFT = 1 << 0,
    COMPANION_REPORT_MASTER     = 1 << 1,
};

enum companion_lock_flags {
    COMPANION_LOCK_CAPS   = 1 << 0,
    COMPANION_LOCK_NUM    = 1 << 1,
    COMPANION_LOCK_SCROLL = 1 << 2,
};

enum companion_trackball_flags {
    COMPANION_TB_LEFT_HELD     = 1 << 0,
    COMPANION_TB_LEFT_LATCHED  = 1 << 1,
    COMPANION_TB_RIGHT_HELD    = 1 << 2,
    COMPANION_TB_RIGHT_LATCHED = 1 << 3,
};

enum companion_usb_flags {
    COMPANION_USB_HOST_CONNECTED = 1 << 0,
    COMPANION_USB_SUSPENDED      = 1 << 1,
    COMPANION_USB_STARTUP_ACTIVE = 1 << 2,
};

enum companion_split_flags {
    COMPANION_SPLIT_TRANSPORT_CONNECTED = 1 << 0,
    COMPANION_SPLIT_LOCAL_LEFT          = 1 << 1,
    COMPANION_SPLIT_MASTER              = 1 << 2,
};

enum companion_sensor_flags {
    COMPANION_SENSOR_VALID  = 1 << 0,
    COMPANION_SENSOR_LIFTED = 1 << 1,
};

enum companion_lift_flags {
    COMPANION_LIFT_LEFT  = 1 << 0,
    COMPANION_LIFT_RIGHT = 1 << 1,
};

enum companion_status_flags {
    COMPANION_STATUS_ENABLED = 1 << 0,
};

enum companion_set_config_flags {
    COMPANION_SET_CONFIG_RESET_DEFAULTS        = 1 << 0,
    COMPANION_SET_CONFIG_RESET_SCROLL_DEFAULTS = 1 << 1,
    COMPANION_SET_CONFIG_RESET_CPI_DEFAULTS    = 1 << 2,
    COMPANION_SET_CONFIG_UPDATE_SCROLL         = 1 << 3,
    COMPANION_SET_CONFIG_UPDATE_CPI            = 1 << 4,
};

enum companion_layer_color_flags {
    COMPANION_LAYER_COLOR_ALTERNATE = 1 << 0,
};

typedef struct {
    uint32_t magic;
    uint8_t  version;
    uint8_t  count;
    uint8_t  reserved[2];
    uint8_t  scroll_divisors[TB_CPI_LEVEL_COUNT];
    uint16_t cpi_levels[TB_CPI_LEVEL_COUNT];
} trackball_level_eeprom_config_t;

_Static_assert(sizeof(trackball_level_eeprom_config_t) <= EECONFIG_USER_DATA_SIZE, "NPE633 user EEPROM block is too small for level tables");

enum companion_status_color_id {
    COMPANION_COLOR_OFF = 0,
    COMPANION_COLOR_DEFAULT,
    COMPANION_COLOR_STARTUP,
    COMPANION_COLOR_SLEEP,
    COMPANION_COLOR_NO_HOST,
    COMPANION_COLOR_BOOT_SIGNAL,
    COMPANION_COLOR_ERROR,
    COMPANION_COLOR_CAPS_LOCK,
    COMPANION_COLOR_NUM_LOCK,
    COMPANION_COLOR_SCROLL_LOCK,
    COMPANION_COLOR_PRECISION,
    COMPANION_COLOR_TRACKBALL_MODE_BASE = 0x20,
};

static bool     status_enabled          = true;
static uint8_t  status_brightness       = 64;
static uint32_t status_startup_timer    = 0;
static uint32_t status_error_timer      = 0;
static uint32_t status_error_check_timer = 0;
static uint8_t  status_error_code       = STATUS_ERROR_NONE;
static bool                  status_initialized          = false;
static bool                  status_suspended            = false;
static bool                  status_seen_configured_host = false;
static usb_configure_state_t status_last_usb_state       = USB_DEVICE_STATE_NO_INIT;
static uint32_t              status_usb_unconfigured_timer = 0;
static uint32_t              status_usb_recovery_timer     = 0;
static bool                  status_caps_lock            = false;
static bool                  status_num_lock             = false;
static bool                  status_scroll_lock          = false;
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

#define TB_SCROLL_DEFAULT_DIVISORS {192, 128, 96, TB_SCROLL_DEFAULT_DIVISOR, 40, 24, 12}
#define TB_CPI_DEFAULT_LEVELS {500, 750, 1000, TB_CPI_DEFAULT, 3000, 5000, 8000}

static const uint8_t  trackball_scroll_default_divisors[] = TB_SCROLL_DEFAULT_DIVISORS;
static uint8_t        trackball_scroll_divisors[]         = TB_SCROLL_DEFAULT_DIVISORS;
static uint8_t        trackball_scroll_levels[]           = {
    [TB_SIDE_LEFT]  = TB_SCROLL_DEFAULT_LEVEL,
    [TB_SIDE_RIGHT] = TB_SCROLL_DEFAULT_LEVEL,
};
static const uint16_t trackball_cpi_default_levels[]    = TB_CPI_DEFAULT_LEVELS;
static uint16_t       trackball_cpi_levels[]            = TB_CPI_DEFAULT_LEVELS;
static uint8_t        trackball_cpi_level_by_side[]     = {
    [TB_SIDE_LEFT]  = TB_CPI_DEFAULT_LEVEL,
    [TB_SIDE_RIGHT] = TB_CPI_DEFAULT_LEVEL,
};
static bool           trackball_precision_active = false;
static companion_sensor_state_t companion_local_sensor_state  = {0};
static companion_sensor_state_t companion_remote_sensor_state = {0};
static uint32_t                 companion_remote_poll_timer   = 0;
static companion_state_t        companion_last_state          = {0};
static bool                     companion_last_state_valid    = false;
static layer_state_t            companion_pending_layer_state = 0;
static bool                     companion_layer_state_pending = false;
static uint8_t                  companion_sequence            = 0;
static bool                     companion_query_status_pending = false;
static bool                     companion_query_config_pending = false;
static bool                     companion_query_class_pending  = false;
static uint8_t                  companion_query_class          = COMPANION_CLASS_IDENTITY;

static uint8_t status_active_layer_from_state(layer_state_t active_layer_state);
static void status_render(void);
static void status_render_for_layer_state(layer_state_t active_layer_state);
static void status_show_level_overlay(enum status_level_overlay_kind kind, uint8_t level);
static status_hsv_t status_color_for_trackball_mode(enum trackball_mode mode);
static void status_invalidate_frame_cache(void);
static void status_usb_recovery_task(void);
static void companion_invalidate_report_cache(void);
static void companion_queue_layer_state(layer_state_t active_layer_state);
static void companion_send_layer_state(layer_state_t active_layer_state);
static void companion_flush_pending_layer_state(void);
static void companion_task(void);
static void companion_send_report(uint8_t event_class, uint8_t flags, const uint8_t *payload, uint8_t payload_len);
static void trackball_apply_cpi(void);
static void trackball_clear_scroll_accums(void);

static const status_layer_style_t status_layer_styles[] = {
    [_BASE] = {{HSV_WHITE}, {HSV_WHITE}, false, STATUS_LAYER_BRIGHTNESS_DIM},
    [_SH2]  = {{HSV_WHITE}, {HSV_BLUE}, true, STATUS_LAYER_BRIGHTNESS_FULL},
    [_MOU]  = {{HSV_YELLOW}, {HSV_YELLOW}, false, STATUS_LAYER_BRIGHTNESS_FULL},
    [_NAV]  = {{HSV_GREEN}, {HSV_GREEN}, false, STATUS_LAYER_BRIGHTNESS_FULL},
    [_NUM]  = {{HSV_BLUE}, {HSV_BLUE}, false, STATUS_LAYER_BRIGHTNESS_FULL},
    [_FN1]  = {{HSV_PURPLE}, {HSV_PURPLE}, false, STATUS_LAYER_BRIGHTNESS_FULL},
    [_FN2]  = {{HSV_PURPLE}, {HSV_CYAN}, true, STATUS_LAYER_BRIGHTNESS_FULL},
    [_SYS]  = {{HSV_PURPLE}, {HSV_YELLOW}, true, STATUS_LAYER_BRIGHTNESS_FULL},
    [_GAME] = {{HSV_RED}, {HSV_ORANGE}, true, STATUS_LAYER_BRIGHTNESS_FULL},
};

static const char companion_layer_aliases[_LAYER_COUNT][6] = {
    [_BASE] = "BASE",
    [_SH2]  = "SH2",
    [_MOU]  = "MOU",
    [_NAV]  = "NAV",
    [_NUM]  = "NUM",
    [_FN1]  = "FN1",
    [_FN2]  = "FN2",
    [_SYS]  = "SYS",
    [_GAME] = "GAME",
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
        MO(_SH2), KC_SCLN , KC_COMM , KC_DOT  , KC_P    , KC_Y    , KC_TAB  ,                     GAME_TOG, KC_F    , KC_G    , KC_C    , KC_R    , KC_L    , MO(_SH2),
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
        KC_TRNS      , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        ,                               SCRL_DOWN    , CPI_DOWN     , LCLK         , RCLK         , SCRL_MOD     , TB_R_SMRT_KEY, KC_TRNS,
        KC_TRNS      , KC_NO        , KC_NO        , KC_NO        , KC_NO        , KC_NO        , SCRL_MOD     ,                               SCRL_DFLT    , CPI_DFLT     , TB_L_ZOOM_KEY, MCLK         , TB_R_SMRT_KEY, TB_R_PAN_KEY , KC_TRNS,
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
        KC_TRNS, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_TRNS,                   KC_NO  , KC_NO  , KC_P7  , KC_P8  , KC_P9  , KC_PPLS, KC_TRNS,
        KC_TRNS, KC_LSFT, KC_LCTL, KC_LALT, KC_LGUI, KC_NO  , KC_NO  ,                   KC_NO  , KC_NO  , KC_P4  , KC_P5  , KC_P6  , KC_PPLS, KC_TRNS,
        KC_TRNS, KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  , KC_NO  ,                   KC_NO  , KC_NO  , KC_P1  , KC_P2  , KC_P3  , KC_PENT, KC_TRNS,
                          KC_TRNS, KC_TRNS, KC_TRNS,                                                       KC_TRNS, KC_P0  , KC_PDOT,
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
    ),

    [_GAME] = LAYOUT(
        KC_ESC  , KC_NO   , KC_1    , KC_2    , KC_3    , KC_4    , KC_5    ,                     TO_BASE , KC_6    , KC_7    , KC_8    , KC_9    , KC_0    , KC_BSPC,
        KC_NO   , KC_TAB  , KC_Q    , KC_W    , KC_E    , KC_R    , KC_T    ,                     GAME_TOG, KC_Y    , KC_U    , KC_I    , KC_O    , KC_P    , KC_BSLS,
        KC_NO   , KC_LSFT , KC_A    , KC_S    , KC_D    , KC_F    , KC_G    ,                     KC_NO   , KC_H    , KC_J    , KC_K    , KC_L    , KC_SCLN , KC_QUOT,
        KC_NO   , KC_LCTL , KC_Z    , KC_X    , KC_C    , KC_V    , KC_B    ,                     KC_NO   , KC_N    , KC_M    , KC_COMM , KC_DOT  , KC_SLSH , KC_RCTL,
                            KC_LALT , KC_LGUI , KC_SPC  ,                                                             TT(_MOU), KC_RGUI , KC_RALT,
                                                KC_ENT  ,                                                             KC_SPC,
                                                          RCLK    ,                                         TT(_FN1),
                                                                    MCLK    ,                     TT(_NAV)
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

static bool trackball_target_includes_side(enum trackball_target target, enum trackball_side side) {
    return (target & (1 << side)) != 0;
}

static void trackball_clear_scroll_accums_for_target(enum trackball_target target) {
    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        if (trackball_target_includes_side(target, side)) {
            trackball_states[side].v_accum = 0;
            trackball_states[side].h_accum = 0;
        }
    }
}

static uint8_t trackball_scroll_level_for_side(enum trackball_side side) {
    return trackball_scroll_levels[side];
}

static uint8_t trackball_cpi_level_for_side(enum trackball_side side) {
    return trackball_cpi_level_by_side[side];
}

static uint8_t trackball_scroll_divisor_for_side(enum trackball_side side) {
    return trackball_scroll_divisors[trackball_scroll_level_for_side(side)];
}

static uint16_t trackball_cpi_value_for_side(enum trackball_side side) {
    return trackball_cpi_levels[trackball_cpi_level_for_side(side)];
}

static bool trackball_level_active_on_any_side(const uint8_t levels[], uint8_t level) {
    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        if (levels[side] == level) {
            return true;
        }
    }

    return false;
}

static uint8_t trackball_overlay_level_for_target(enum trackball_target target, const uint8_t levels[]) {
    if (trackball_target_includes_side(target, TB_SIDE_LEFT)) {
        return levels[TB_SIDE_LEFT];
    }

    return levels[TB_SIDE_RIGHT];
}

static uint16_t trackball_sanitize_cpi(uint16_t cpi) {
    if (cpi < TB_CPI_MIN) {
        cpi = TB_CPI_MIN;
    } else if (cpi > TB_CPI_MAX) {
        cpi = TB_CPI_MAX;
    }

    uint16_t remainder = cpi % TB_CPI_STEP;
    if (remainder >= TB_CPI_STEP / 2) {
        cpi += TB_CPI_STEP - remainder;
    } else {
        cpi -= remainder;
    }

    if (cpi < TB_CPI_MIN) {
        return TB_CPI_MIN;
    }

    if (cpi > TB_CPI_MAX) {
        return TB_CPI_MAX;
    }

    return cpi;
}

static uint8_t trackball_sanitize_scroll_divisor(uint8_t divisor) {
    if (divisor < TB_SCROLL_DIVISOR_MIN) {
        return TB_SCROLL_DIVISOR_MIN;
    }

    if (divisor > TB_SCROLL_DIVISOR_MAX) {
        return TB_SCROLL_DIVISOR_MAX;
    }

    return divisor;
}

static void trackball_load_default_level_tables(void) {
    for (uint8_t level = 0; level < TB_CPI_LEVEL_COUNT; level++) {
        trackball_scroll_divisors[level] = trackball_scroll_default_divisors[level];
        trackball_cpi_levels[level] = trackball_cpi_default_levels[level];
    }
}

static void trackball_store_level_tables(void) {
    trackball_level_eeprom_config_t config = {
        .magic   = TB_LEVEL_EEPROM_MAGIC,
        .version = TB_LEVEL_EEPROM_VERSION,
        .count   = TB_CPI_LEVEL_COUNT,
    };

    for (uint8_t level = 0; level < TB_CPI_LEVEL_COUNT; level++) {
        config.scroll_divisors[level] = trackball_sanitize_scroll_divisor(trackball_scroll_divisors[level]);
        config.cpi_levels[level] = trackball_sanitize_cpi(trackball_cpi_levels[level]);
    }

    eeconfig_update_user_datablock(&config, 0, sizeof(config));
}

static bool trackball_read_stored_level_tables(void) {
    trackball_level_eeprom_config_t config = {0};

    if (!eeconfig_is_user_datablock_valid()) {
        return false;
    }

    eeconfig_read_user_datablock(&config, 0, sizeof(config));
    if (config.magic != TB_LEVEL_EEPROM_MAGIC || config.version != TB_LEVEL_EEPROM_VERSION || config.count != TB_CPI_LEVEL_COUNT) {
        return false;
    }

    for (uint8_t level = 0; level < TB_CPI_LEVEL_COUNT; level++) {
        trackball_scroll_divisors[level] = trackball_sanitize_scroll_divisor(config.scroll_divisors[level]);
        trackball_cpi_levels[level] = trackball_sanitize_cpi(config.cpi_levels[level]);
    }

    return true;
}

static void trackball_initialize_level_tables(void) {
    if (trackball_read_stored_level_tables()) {
        return;
    }

    trackball_load_default_level_tables();
    trackball_store_level_tables();
}

static bool trackball_update_level_table(uint8_t visible_level, uint8_t update_flags, uint8_t scroll_divisor, uint16_t cpi) {
    if (visible_level == 0 || visible_level > TB_CPI_LEVEL_COUNT) {
        return false;
    }

    uint8_t level = visible_level - 1;
    bool update_scroll = (update_flags & COMPANION_SET_CONFIG_UPDATE_SCROLL) != 0;
    bool update_cpi    = (update_flags & COMPANION_SET_CONFIG_UPDATE_CPI) != 0;

    if (!update_scroll && !update_cpi) {
        update_scroll = true;
        update_cpi    = true;
    }

    if (update_scroll) {
        trackball_scroll_divisors[level] = trackball_sanitize_scroll_divisor(scroll_divisor);
    }

    if (update_cpi) {
        trackball_cpi_levels[level] = trackball_sanitize_cpi(cpi);
    }

    trackball_store_level_tables();

    if (update_cpi && trackball_level_active_on_any_side(trackball_cpi_level_by_side, level) && !trackball_precision_active) {
        trackball_apply_cpi();
    }

    if (update_scroll && trackball_level_active_on_any_side(trackball_scroll_levels, level)) {
        trackball_clear_scroll_accums();
    }

    companion_invalidate_report_cache();
    return true;
}

static void trackball_reset_scroll_divisors(void) {
    for (uint8_t level = 0; level < TB_CPI_LEVEL_COUNT; level++) {
        trackball_scroll_divisors[level] = trackball_scroll_default_divisors[level];
    }

    trackball_store_level_tables();
    trackball_clear_scroll_accums();
    companion_invalidate_report_cache();
}

static void trackball_reset_cpi_levels(void) {
    for (uint8_t level = 0; level < TB_CPI_LEVEL_COUNT; level++) {
        trackball_cpi_levels[level] = trackball_cpi_default_levels[level];
    }

    trackball_store_level_tables();
    if (!trackball_precision_active) {
        trackball_apply_cpi();
    }

    companion_invalidate_report_cache();
}

static void trackball_reset_level_tables(void) {
    trackball_load_default_level_tables();
    trackball_store_level_tables();

    if (!trackball_precision_active) {
        trackball_apply_cpi();
    }

    trackball_clear_scroll_accums();
    companion_invalidate_report_cache();
}

static void trackball_scroll_speed_up(enum trackball_target target) {
    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        if (trackball_target_includes_side(target, side) && trackball_scroll_levels[side] < ARRAY_SIZE(trackball_scroll_divisors) - 1) {
            trackball_scroll_levels[side]++;
            trackball_clear_scroll_accums_for_target((enum trackball_target)(1 << side));
        }
    }

    status_show_level_overlay(STATUS_LEVEL_SCROLL, trackball_overlay_level_for_target(target, trackball_scroll_levels));
}

static void trackball_scroll_speed_down(enum trackball_target target) {
    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        if (trackball_target_includes_side(target, side) && trackball_scroll_levels[side] > 0) {
            trackball_scroll_levels[side]--;
            trackball_clear_scroll_accums_for_target((enum trackball_target)(1 << side));
        }
    }

    status_show_level_overlay(STATUS_LEVEL_SCROLL, trackball_overlay_level_for_target(target, trackball_scroll_levels));
}

static void trackball_scroll_speed_default(enum trackball_target target) {
    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        if (trackball_target_includes_side(target, side)) {
            trackball_scroll_levels[side] = TB_SCROLL_DEFAULT_LEVEL;
            trackball_clear_scroll_accums_for_target((enum trackball_target)(1 << side));
        }
    }

    status_show_level_overlay(STATUS_LEVEL_SCROLL, trackball_overlay_level_for_target(target, trackball_scroll_levels));
}

static void trackball_set_side_cpi(enum trackball_side side, uint16_t cpi) {
#if defined(SPLIT_POINTING_ENABLE) && defined(POINTING_DEVICE_COMBINED)
    pointing_device_set_cpi_on_side(side == TB_SIDE_LEFT, cpi);
#else
    (void)side;
    pointing_device_set_cpi(cpi);
#endif
}

static bool trackball_mode_uses_scroll_cpi(enum trackball_mode mode) {
    switch (mode) {
        case TB_MODE_SMART_SCROLL:
        case TB_MODE_VSCROLL:
        case TB_MODE_HSCROLL:
        case TB_MODE_PAN:
            return true;

        case TB_MODE_CURSOR:
        case TB_MODE_VOLUME:
        case TB_MODE_BRIGHTNESS:
        case TB_MODE_ZOOM:
        case TB_MODE_ROTATE:
            return false;
    }

    return false;
}

static uint16_t trackball_cpi_for_side(enum trackball_side side) {
    if (trackball_precision_active) {
        return TB_CPI_PRECISION;
    }

    if (trackball_mode_uses_scroll_cpi(trackball_effective_mode(side))) {
        return TB_SCROLL_MODE_CPI;
    }

    return trackball_cpi_value_for_side(side);
}

static void trackball_apply_cpi(void) {
    trackball_set_side_cpi(TB_SIDE_LEFT, trackball_cpi_for_side(TB_SIDE_LEFT));
    trackball_set_side_cpi(TB_SIDE_RIGHT, trackball_cpi_for_side(TB_SIDE_RIGHT));
}

static void trackball_cpi_up(enum trackball_target target) {
    bool changed = false;

    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        if (trackball_target_includes_side(target, side) && trackball_cpi_level_by_side[side] < ARRAY_SIZE(trackball_cpi_levels) - 1) {
            trackball_cpi_level_by_side[side]++;
            changed = true;
        }
    }

    if (changed && !trackball_precision_active) {
        trackball_apply_cpi();
    }

    status_show_level_overlay(STATUS_LEVEL_CPI, trackball_overlay_level_for_target(target, trackball_cpi_level_by_side));
}

static void trackball_cpi_down(enum trackball_target target) {
    bool changed = false;

    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        if (trackball_target_includes_side(target, side) && trackball_cpi_level_by_side[side] > 0) {
            trackball_cpi_level_by_side[side]--;
            changed = true;
        }
    }

    if (changed && !trackball_precision_active) {
        trackball_apply_cpi();
    }

    status_show_level_overlay(STATUS_LEVEL_CPI, trackball_overlay_level_for_target(target, trackball_cpi_level_by_side));
}

static void trackball_cpi_default(enum trackball_target target) {
    bool changed = false;

    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        if (trackball_target_includes_side(target, side)) {
            changed |= trackball_cpi_level_by_side[side] != TB_CPI_DEFAULT_LEVEL;
            trackball_cpi_level_by_side[side] = TB_CPI_DEFAULT_LEVEL;
        }
    }

    if (changed && !trackball_precision_active) {
        trackball_apply_cpi();
    }

    status_show_level_overlay(STATUS_LEVEL_CPI, trackball_overlay_level_for_target(target, trackball_cpi_level_by_side));
}

static void trackball_precision_set(bool active) {
    trackball_precision_active = active;
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

static mouse_hv_report_t trackball_scroll_value(enum trackball_side side, int16_t *accum, int16_t delta) {
    mouse_hv_report_t output = 0;
    uint8_t           divisor = trackball_scroll_divisor_for_side(side);

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
                report.v = trackball_scroll_value(side, &state->v_accum, -report.y);
                report.x = 0;
                report.y = 0;
            } else if (state->axis_lock == TB_AXIS_HORIZONTAL) {
                report.h = trackball_scroll_value(side, &state->h_accum, report.x);
                report.x = 0;
                report.y = 0;
            } else {
                report.x = 0;
                report.y = 0;
            }
            break;

        case TB_MODE_VSCROLL:
            report.v = trackball_scroll_value(side, &state->v_accum, -report.y);
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_HSCROLL:
            report.h = trackball_scroll_value(side, &state->h_accum, report.x);
            report.x = 0;
            report.y = 0;
            break;

        case TB_MODE_PAN:
            report.h = trackball_scroll_value(side, &state->h_accum, report.x);
            report.v = trackball_scroll_value(side, &state->v_accum, -report.y);
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

static void companion_sensor_state_slave_handler(uint8_t initiator2target_buffer_size, const void *initiator2target_buffer, uint8_t target2initiator_buffer_size, void *target2initiator_buffer) {
    (void)initiator2target_buffer_size;
    (void)initiator2target_buffer;

    if (target2initiator_buffer_size != sizeof(companion_sensor_state_t)) {
        return;
    }

    *(companion_sensor_state_t *)target2initiator_buffer = companion_local_sensor_state;
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

static bool status_usb_ready_for_frames(void) {
    usb_configure_state_t usb_state = usb_device_state_get_configure_state();
    return usb_state == USB_DEVICE_STATE_CONFIGURED || status_suspended || status_seen_configured_host;
}

static void status_invalidate_frame_cache(void) {
    uint32_t now = timer_read32();

    status_last_local_frame_valid  = false;
    status_last_remote_frame_valid = false;
    status_local_refresh_timer     = now;
    status_remote_refresh_timer    = now;
}

static bool status_usb_recovery_should_run(usb_configure_state_t usb_state) {
#if defined(SPLIT_KEYBOARD)
    if (!is_keyboard_master()) {
        return false;
    }
#endif

    return status_seen_configured_host && !status_suspended && usb_state != USB_DEVICE_STATE_CONFIGURED;
}

static void status_usb_recovery_note_configured(void) {
    status_seen_configured_host    = true;
    status_usb_unconfigured_timer = 0;
    status_usb_recovery_timer     = 0;
}

static void status_usb_recovery_task(void) {
#ifdef PROTOCOL_CHIBIOS
    usb_configure_state_t usb_state = usb_device_state_get_configure_state();
    uint32_t              now       = timer_read32();

    if (usb_state == USB_DEVICE_STATE_CONFIGURED) {
        status_usb_recovery_note_configured();
        return;
    }

    if (!status_usb_recovery_should_run(usb_state)) {
        status_usb_unconfigured_timer = 0;
        return;
    }

    if (status_usb_unconfigured_timer == 0) {
        status_usb_unconfigured_timer = now;
        return;
    }

    if (timer_elapsed32(status_usb_unconfigured_timer) < STATUS_USB_RECOVERY_STUCK_MS) {
        return;
    }

    if (status_usb_recovery_timer != 0 && timer_elapsed32(status_usb_recovery_timer) < STATUS_USB_RECOVERY_RETRY_MS) {
        return;
    }

    status_usb_recovery_timer     = now;
    status_usb_unconfigured_timer = now;
    status_invalidate_frame_cache();
    companion_invalidate_report_cache();
    restart_usb_driver(&USB_DRIVER);
#endif
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

static uint8_t status_active_layer_from_state(layer_state_t active_layer_state) {
    uint8_t layer = get_highest_layer(active_layer_state | default_layer_state);

    if (layer >= ARRAY_SIZE(status_layer_styles)) {
        return _BASE;
    }

    return layer;
}

static status_model_t status_collect_model_for_layer_state(uint32_t now, layer_state_t active_layer_state) {
    uint8_t               layer     = status_active_layer_from_state(active_layer_state);
    usb_configure_state_t usb_state = usb_device_state_get_configure_state();

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
        .cpi_level        = trackball_cpi_level_for_side(TB_SIDE_LEFT),
        .left_mode        = trackball_effective_mode(TB_SIDE_LEFT),
        .right_mode       = trackball_effective_mode(TB_SIDE_RIGHT),
    };
}

static status_model_t status_collect_model(uint32_t now) {
    return status_collect_model_for_layer_state(now, layer_state);
}


static uint8_t companion_lock_flags_for_model(status_model_t model) {
    uint8_t flags = 0;

    if (model.caps_lock) {
        flags |= COMPANION_LOCK_CAPS;
    }

    if (model.num_lock) {
        flags |= COMPANION_LOCK_NUM;
    }

    if (model.scroll_lock) {
        flags |= COMPANION_LOCK_SCROLL;
    }

    return flags;
}

static uint8_t companion_trackball_mode_flags(void) {
    uint8_t flags = 0;

    if (trackball_states[TB_SIDE_LEFT].held_mode != TB_MODE_CURSOR) {
        flags |= COMPANION_TB_LEFT_HELD;
    }

    if (trackball_states[TB_SIDE_LEFT].latched_mode != TB_MODE_CURSOR) {
        flags |= COMPANION_TB_LEFT_LATCHED;
    }

    if (trackball_states[TB_SIDE_RIGHT].held_mode != TB_MODE_CURSOR) {
        flags |= COMPANION_TB_RIGHT_HELD;
    }

    if (trackball_states[TB_SIDE_RIGHT].latched_mode != TB_MODE_CURSOR) {
        flags |= COMPANION_TB_RIGHT_LATCHED;
    }

    return flags;
}

static companion_sensor_state_t companion_sensor_state_for_side(enum trackball_side side) {
#ifdef SPLIT_KEYBOARD
    if (status_side_is_local(side)) {
        return companion_local_sensor_state;
    }

    return companion_remote_sensor_state;
#else
    (void)side;
    return companion_local_sensor_state;
#endif
}

static void companion_refresh_remote_sensor_state(uint32_t now) {
#if defined(SPLIT_KEYBOARD) && defined(SPLIT_TRANSACTION_RPC)
    if (!is_keyboard_master()) {
        return;
    }

    if (!is_transport_connected()) {
        companion_remote_sensor_state = (companion_sensor_state_t){0};
        return;
    }

    if (timer_elapsed32(companion_remote_poll_timer) < COMPANION_REMOTE_POLL_MS) {
        return;
    }

    companion_remote_poll_timer = now;
    companion_sensor_state_t remote_state = {0};
    if (transaction_rpc_recv(RPC_ID_REMOTE_SENSOR_STATE, sizeof(remote_state), &remote_state)) {
        companion_remote_sensor_state = remote_state;
    } else {
        companion_remote_sensor_state = (companion_sensor_state_t){0};
    }
#else
    (void)now;
#endif
}

static companion_state_t companion_collect_state(uint32_t now) {
    status_model_t model = status_collect_model(now);
    companion_sensor_state_t left_sensor  = companion_sensor_state_for_side(TB_SIDE_LEFT);
    companion_sensor_state_t right_sensor = companion_sensor_state_for_side(TB_SIDE_RIGHT);
    uint8_t lift_flags = 0;
    uint8_t lift_valid_flags = 0;

    if (left_sensor.flags & COMPANION_SENSOR_VALID) {
        lift_valid_flags |= COMPANION_LIFT_LEFT;
        if (left_sensor.flags & COMPANION_SENSOR_LIFTED) {
            lift_flags |= COMPANION_LIFT_LEFT;
        }
    }

    if (right_sensor.flags & COMPANION_SENSOR_VALID) {
        lift_valid_flags |= COMPANION_LIFT_RIGHT;
        if (right_sensor.flags & COMPANION_SENSOR_LIFTED) {
            lift_flags |= COMPANION_LIFT_RIGHT;
        }
    }

    companion_state_t state = {
        .active_layer         = model.active_layer,
        .lock_flags           = companion_lock_flags_for_model(model),
        .left_mode            = model.left_mode,
        .right_mode           = model.right_mode,
        .trackball_mode_flags = companion_trackball_mode_flags(),
        .scroll_level         = trackball_scroll_level_for_side(TB_SIDE_LEFT),
        .scroll_divisor       = trackball_scroll_divisor_for_side(TB_SIDE_LEFT),
        .cpi_level            = trackball_cpi_level_for_side(TB_SIDE_LEFT),
        .cpi_value            = model.precision_active ? TB_CPI_PRECISION : trackball_cpi_value_for_side(TB_SIDE_LEFT),
        .right_scroll_level   = trackball_scroll_level_for_side(TB_SIDE_RIGHT),
        .right_scroll_divisor = trackball_scroll_divisor_for_side(TB_SIDE_RIGHT),
        .right_cpi_level      = trackball_cpi_level_for_side(TB_SIDE_RIGHT),
        .right_cpi_value      = model.precision_active ? TB_CPI_PRECISION : trackball_cpi_value_for_side(TB_SIDE_RIGHT),
        .precision_active     = model.precision_active,
        .startup_active       = model.startup_active,
        .suspended            = model.suspended,
        .host_connected       = model.host_connected,
        .error_code           = model.error_code,
        .transport_connected  = true,
        .pointing_status      = POINTING_DEVICE_STATUS_SUCCESS,
        .local_side_left      = true,
        .master               = true,
        .status_enabled       = status_enabled,
        .status_brightness    = status_brightness,
        .lift_flags           = lift_flags,
        .lift_valid_flags     = lift_valid_flags,
        .left_motion          = left_sensor.motion,
        .right_motion         = right_sensor.motion,
    };

#ifdef SPLIT_KEYBOARD
    state.transport_connected = is_transport_connected();
    state.local_side_left     = is_keyboard_left();
    state.master              = is_keyboard_master();
#endif

#ifdef POINTING_DEVICE_ENABLE
    state.pointing_status = pointing_device_get_status();
#endif

    return state;
}

static bool companion_report_ready(void) {
#ifdef SPLIT_KEYBOARD
    if (!is_keyboard_master()) {
        return false;
    }
#endif

    return usb_device_state_get_configure_state() == USB_DEVICE_STATE_CONFIGURED;
}

static void companion_invalidate_report_cache(void) {
    companion_last_state_valid = false;
}

static void companion_send_hsv_payload(uint8_t event_class, uint8_t flags, uint8_t id, status_hsv_t color, uint8_t aux) {
    uint8_t payload[] = {id, color.h, color.s, color.v, aux};
    companion_send_report(event_class, flags, payload, sizeof(payload));
}

static void companion_send_layer_colors(uint8_t flags) {
    for (uint8_t layer = 0; layer < ARRAY_SIZE(status_layer_styles); layer++) {
        status_layer_style_t style = status_layer_styles[layer];
        uint8_t payload[] = {
            layer,
            style.primary.h,
            style.primary.s,
            style.primary.v,
            style.secondary.h,
            style.secondary.s,
            style.secondary.v,
            style.alternate ? COMPANION_LAYER_COLOR_ALTERNATE : 0,
            style.brightness_percent,
        };
        companion_send_report(COMPANION_CLASS_LAYER_COLOR, flags, payload, sizeof(payload));
    }
}

static void companion_send_layer_aliases(uint8_t flags) {
    for (uint8_t layer = 0; layer < ARRAY_SIZE(companion_layer_aliases); layer++) {
        uint8_t payload[COMPANION_PAYLOAD_SIZE] = {0};
        uint8_t alias_len = 0;

        while (alias_len < sizeof(companion_layer_aliases[layer]) && companion_layer_aliases[layer][alias_len] != '\0') {
            alias_len++;
        }

        payload[0] = layer;
        payload[1] = alias_len;

        for (uint8_t i = 0; i < alias_len && i < COMPANION_PAYLOAD_SIZE - 2; i++) {
            payload[2 + i] = companion_layer_aliases[layer][i];
        }

        companion_send_report(COMPANION_CLASS_LAYER_ALIAS, flags, payload, 2 + alias_len);
    }
}

static void companion_send_level_colors(uint8_t flags) {
    for (uint8_t level = 0; level < ARRAY_SIZE(status_level_colors); level++) {
        uint16_t cpi = trackball_cpi_levels[level];
        uint8_t payload[] = {
            level + 1,
            status_level_colors[level].h,
            status_level_colors[level].s,
            status_level_colors[level].v,
            trackball_scroll_divisors[level],
            cpi & 0xFF,
            cpi >> 8,
        };
        companion_send_report(COMPANION_CLASS_LEVEL_TABLE, flags, payload, sizeof(payload));
    }
}

static void companion_send_status_colors(uint8_t flags) {
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_OFF, (status_hsv_t){HSV_OFF}, 0);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_DEFAULT, (status_hsv_t){HSV_WHITE}, 0);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_STARTUP, (status_hsv_t){HSV_WHITE}, 0);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_SLEEP, (status_hsv_t){HSV_BLUE}, STATUS_SLEEP_BREATHE_MIN);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_NO_HOST, (status_hsv_t){HSV_MAGENTA}, STATUS_NO_HOST_BREATHE_MIN);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_BOOT_SIGNAL, (status_hsv_t){HSV_RED}, 0);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_ERROR, (status_hsv_t){HSV_RED}, 0);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_CAPS_LOCK, status_caps_lock_color, 0);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_NUM_LOCK, status_num_lock_color, 0);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_SCROLL_LOCK, status_scroll_lock_color, 0);
    companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_PRECISION, (status_hsv_t){HSV_WHITE}, STATUS_BREATHE_MIN);

    for (uint8_t mode = TB_MODE_CURSOR; mode <= TB_MODE_ROTATE; mode++) {
        companion_send_hsv_payload(COMPANION_CLASS_STATUS_COLOR, flags, COMPANION_COLOR_TRACKBALL_MODE_BASE + mode, status_color_for_trackball_mode(mode), 0);
    }
}

static uint8_t companion_report_flags_for_state(companion_state_t state) {
    uint8_t flags = 0;

    if (state.local_side_left) {
        flags |= COMPANION_REPORT_LOCAL_LEFT;
    }

    if (state.master) {
        flags |= COMPANION_REPORT_MASTER;
    }

    return flags;
}

static void companion_send_report(uint8_t event_class, uint8_t flags, const uint8_t *payload, uint8_t payload_len) {
    uint8_t report[COMPANION_REPORT_SIZE] = {0};

    if (payload_len > COMPANION_PAYLOAD_SIZE) {
        payload_len = COMPANION_PAYLOAD_SIZE;
    }

    report[0] = COMPANION_MAGIC_0;
    report[1] = COMPANION_MAGIC_1;
    report[2] = COMPANION_PROTOCOL_VERSION;
    report[3] = COMPANION_MSG_EVENT;
    report[4] = companion_sequence++;
    report[5] = event_class;
    report[6] = flags;
    report[7] = payload_len;

    for (uint8_t i = 0; i < payload_len; i++) {
        report[COMPANION_PAYLOAD_OFFSET + i] = payload[i];
    }

    raw_hid_send(report, sizeof(report));
}

static void companion_send_class(uint8_t event_class, companion_state_t state) {
    uint8_t payload[COMPANION_PAYLOAD_SIZE] = {0};
    uint8_t payload_len = 0;
    uint8_t flags = companion_report_flags_for_state(state);

    switch (event_class) {
        case COMPANION_CLASS_IDENTITY:
            payload[0] = COMPANION_PROTOCOL_VERSION;
            payload[1] = COMPANION_CLASS_COUNT;
            payload[2] = STATUS_LIGHT_COUNT;
            payload[3] = TB_SCROLL_DEFAULT_LEVEL;
            payload[4] = TB_CPI_DEFAULT_LEVEL;
            payload[5] = _LAYER_COUNT;
            payload_len = 6;
            break;

        case COMPANION_CLASS_LAYER:
            payload[0] = state.active_layer;
            payload_len = 1;
            break;

        case COMPANION_CLASS_LOCKS:
            payload[0] = state.lock_flags;
            payload_len = 1;
            break;

        case COMPANION_CLASS_TRACKBALL:
            payload[0] = state.left_mode;
            payload[1] = state.right_mode;
            payload[2] = state.trackball_mode_flags;
            payload_len = 3;
            break;

        case COMPANION_CLASS_LEVELS:
            payload[0] = state.scroll_level;
            payload[1] = state.cpi_level;
            payload[2] = state.precision_active ? 1 : 0;
            payload[3] = state.right_scroll_level;
            payload[4] = state.right_cpi_level;
            payload[5] = state.scroll_divisor;
            payload[6] = state.right_scroll_divisor;
            payload[7] = state.cpi_value & 0xFF;
            payload[8] = state.cpi_value >> 8;
            payload[9] = state.right_cpi_value & 0xFF;
            payload[10] = state.right_cpi_value >> 8;
            payload_len = 11;
            break;

        case COMPANION_CLASS_USB:
            payload[0] = (state.host_connected ? COMPANION_USB_HOST_CONNECTED : 0) | (state.suspended ? COMPANION_USB_SUSPENDED : 0) | (state.startup_active ? COMPANION_USB_STARTUP_ACTIVE : 0);
            payload_len = 1;
            break;

        case COMPANION_CLASS_SPLIT:
            payload[0] = state.error_code;
            payload[1] = (state.transport_connected ? COMPANION_SPLIT_TRANSPORT_CONNECTED : 0) | (state.local_side_left ? COMPANION_SPLIT_LOCAL_LEFT : 0) | (state.master ? COMPANION_SPLIT_MASTER : 0);
            payload[2] = state.pointing_status;
            payload_len = 3;
            break;

        case COMPANION_CLASS_LIFT:
            payload[0] = state.lift_flags;
            payload[1] = state.lift_valid_flags;
            payload[2] = state.left_motion;
            payload[3] = state.right_motion;
            payload_len = 4;
            break;

        case COMPANION_CLASS_STATUS:
            payload[0] = state.status_enabled ? COMPANION_STATUS_ENABLED : 0;
            payload[1] = state.status_brightness;
            payload_len = 2;
            break;

        case COMPANION_CLASS_LAYER_COLOR:
            companion_send_layer_colors(flags);
            return;

        case COMPANION_CLASS_LEVEL_TABLE:
            companion_send_level_colors(flags);
            return;

        case COMPANION_CLASS_STATUS_COLOR:
            companion_send_status_colors(flags);
            return;

        case COMPANION_CLASS_LAYER_ALIAS:
            companion_send_layer_aliases(flags);
            return;

        default:
            return;
    }

    companion_send_report(event_class, flags, payload, payload_len);
}

static void companion_send_layer_state(layer_state_t active_layer_state) {
    if (!companion_report_ready()) {
        return;
    }

    companion_state_t state = {
        .active_layer    = status_active_layer_from_state(active_layer_state),
        .local_side_left = true,
        .master          = true,
    };

#ifdef SPLIT_KEYBOARD
    state.local_side_left = is_keyboard_left();
    state.master          = is_keyboard_master();
#endif

    if (companion_last_state_valid && companion_last_state.active_layer == state.active_layer) {
        return;
    }

    companion_send_class(COMPANION_CLASS_LAYER, state);

    if (companion_last_state_valid) {
        companion_last_state.active_layer = state.active_layer;
    }
}

static void companion_queue_layer_state(layer_state_t active_layer_state) {
    companion_pending_layer_state = active_layer_state;
    companion_layer_state_pending = true;
}

static void companion_flush_pending_layer_state(void) {
    if (!companion_layer_state_pending) {
        return;
    }

    companion_send_layer_state(companion_pending_layer_state);

    if (companion_report_ready()) {
        companion_layer_state_pending = false;
    }
}

static bool companion_state_class_changed(uint8_t event_class, companion_state_t previous, companion_state_t current) {
    switch (event_class) {
        case COMPANION_CLASS_LAYER:
            return previous.active_layer != current.active_layer;

        case COMPANION_CLASS_LOCKS:
            return previous.lock_flags != current.lock_flags;

        case COMPANION_CLASS_TRACKBALL:
            return previous.left_mode != current.left_mode || previous.right_mode != current.right_mode || previous.trackball_mode_flags != current.trackball_mode_flags;

        case COMPANION_CLASS_LEVELS:
            return previous.scroll_level != current.scroll_level || previous.cpi_level != current.cpi_level || previous.precision_active != current.precision_active || previous.right_scroll_level != current.right_scroll_level || previous.right_cpi_level != current.right_cpi_level || previous.scroll_divisor != current.scroll_divisor || previous.right_scroll_divisor != current.right_scroll_divisor || previous.cpi_value != current.cpi_value || previous.right_cpi_value != current.right_cpi_value;

        case COMPANION_CLASS_USB:
            return previous.host_connected != current.host_connected || previous.suspended != current.suspended || previous.startup_active != current.startup_active;

        case COMPANION_CLASS_SPLIT:
            return previous.error_code != current.error_code || previous.transport_connected != current.transport_connected || previous.pointing_status != current.pointing_status || previous.local_side_left != current.local_side_left || previous.master != current.master;

        case COMPANION_CLASS_LIFT:
            return previous.lift_flags != current.lift_flags || previous.lift_valid_flags != current.lift_valid_flags;

        case COMPANION_CLASS_STATUS:
            return previous.status_enabled != current.status_enabled || previous.status_brightness != current.status_brightness;

        case COMPANION_CLASS_LAYER_COLOR:
        case COMPANION_CLASS_LEVEL_TABLE:
        case COMPANION_CLASS_STATUS_COLOR:
        case COMPANION_CLASS_LAYER_ALIAS:
            return false;

        case COMPANION_CLASS_IDENTITY:
        default:
            return false;
    }
}

static void companion_send_class_range(uint8_t first_class, uint8_t last_class, companion_state_t state) {
    for (uint8_t event_class = first_class; event_class <= last_class && event_class < COMPANION_CLASS_COUNT; event_class++) {
        companion_send_class(event_class, state);
    }
}

static void companion_send_status_classes(companion_state_t state) {
    companion_send_class_range(COMPANION_STATUS_CLASS_FIRST, COMPANION_STATUS_CLASS_LAST, state);
}

static void companion_send_config_classes(companion_state_t state) {
    companion_send_class_range(COMPANION_CONFIG_CLASS_FIRST, COMPANION_CONFIG_CLASS_LAST, state);
}

static void companion_send_changed_classes(companion_state_t state) {
    if (!companion_last_state_valid) {
        companion_send_status_classes(state);
        companion_last_state = state;
        companion_last_state_valid = true;
        return;
    }

    bool changed = false;
    for (uint8_t event_class = COMPANION_CLASS_LAYER; event_class <= COMPANION_STATUS_CLASS_LAST; event_class++) {
        if (companion_state_class_changed(event_class, companion_last_state, state)) {
            companion_send_class(event_class, state);
            changed = true;
        }
    }

    if (changed) {
        companion_last_state = state;
    }
}

static void companion_task(void) {
    if (!companion_report_ready()) {
        return;
    }

    uint32_t now = timer_read32();
    companion_refresh_remote_sensor_state(now);
    companion_state_t state = companion_collect_state(now);

    if (companion_query_status_pending) {
        companion_send_status_classes(state);
        companion_query_status_pending = false;
        companion_last_state = state;
        companion_last_state_valid = true;
    }

    if (companion_query_config_pending) {
        companion_send_config_classes(state);
        companion_query_config_pending = false;
    }

    if (companion_query_class_pending) {
        companion_send_class(companion_query_class, state);
        companion_query_class_pending = false;
    }

    companion_send_changed_classes(state);
}

static void companion_handle_set_config(uint8_t *data) {
    uint8_t event_class = data[5];
    uint8_t flags       = data[6];
    uint8_t payload_len = data[7];
    uint8_t *payload    = data + COMPANION_PAYLOAD_OFFSET;

    switch (event_class) {
        case COMPANION_CLASS_LEVEL_TABLE:
            if (flags & COMPANION_SET_CONFIG_RESET_DEFAULTS) {
                trackball_reset_level_tables();
                companion_query_class = COMPANION_CLASS_LEVEL_TABLE;
                companion_query_class_pending = true;
                return;
            }

            if (flags & COMPANION_SET_CONFIG_RESET_SCROLL_DEFAULTS) {
                trackball_reset_scroll_divisors();
                companion_query_class = COMPANION_CLASS_LEVEL_TABLE;
                companion_query_class_pending = true;
            }

            if (flags & COMPANION_SET_CONFIG_RESET_CPI_DEFAULTS) {
                trackball_reset_cpi_levels();
                companion_query_class = COMPANION_CLASS_LEVEL_TABLE;
                companion_query_class_pending = true;
            }

            if (payload_len < 7) {
                return;
            }

            if (trackball_update_level_table(payload[0], flags, payload[4], (uint16_t)payload[5] | ((uint16_t)payload[6] << 8))) {
                companion_query_class = COMPANION_CLASS_LEVEL_TABLE;
                companion_query_class_pending = true;
            }
            return;

        default:
            return;
    }
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

void eeconfig_init_user(void) {
    trackball_load_default_level_tables();
    trackball_store_level_tables();
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

static void status_render_for_layer_state(layer_state_t active_layer_state) {
    if (!status_initialized) {
        return;
    }

    if (!status_usb_ready_for_frames() && !status_suspended) {
        return;
    }

    uint32_t now = timer_read32();

    if (!status_enabled) {
        status_apply_all((status_hsv_t){HSV_OFF});
        return;
    }

    status_model_t model = status_collect_model_for_layer_state(now, active_layer_state);

    if (!model.startup_active && !model.suspended && model.host_connected) {
        status_update_error_code(now);
        model.error_code = status_error_code;
    }

    status_frame_t left_frame  = status_build_side_frame(now, model, TB_SIDE_LEFT);
    status_frame_t right_frame = status_build_side_frame(now, model, TB_SIDE_RIGHT);
    status_apply_both_frames(left_frame, right_frame);
}

static void status_render(void) {
    status_render_for_layer_state(layer_state);
}

void keyboard_post_init_user(void) {
#if defined(SPLIT_KEYBOARD) && defined(SPLIT_TRANSACTION_RPC)
    transaction_register_rpc(RPC_ID_STATUS_FRAME, status_frame_slave_handler);
    transaction_register_rpc(RPC_ID_REMOTE_SENSOR_STATE, companion_sensor_state_slave_handler);
#endif

    rgblight_enable_noeeprom();
    rgblight_mode_noeeprom(RGBLIGHT_MODE_STATIC_LIGHT);
    status_startup_timer         = timer_read32();
    status_error_timer           = status_startup_timer;
    status_error_check_timer     = status_startup_timer;
    status_last_usb_state        = usb_device_state_get_configure_state();
    status_seen_configured_host = status_last_usb_state == USB_DEVICE_STATE_CONFIGURED;
    status_initialized           = true;
    status_invalidate_frame_cache();
    trackball_initialize_level_tables();
    trackball_apply_cpi();
    trackball_clear_all();
}

void notify_usb_device_state_change_user(struct usb_device_state usb_state) {
    bool state_changed = usb_state.configure_state != status_last_usb_state;

    status_last_usb_state = usb_state.configure_state;
    status_suspended      = usb_state.configure_state == USB_DEVICE_STATE_SUSPEND;

    if (usb_state.configure_state == USB_DEVICE_STATE_CONFIGURED) {
        status_usb_recovery_note_configured();
    }

    if (status_initialized) {
        if (state_changed) {
            status_invalidate_frame_cache();
            companion_invalidate_report_cache();
        }

        status_render();
    }
}

void suspend_power_down_user(void) {
    status_suspended = true;
    status_invalidate_frame_cache();
    status_render();
}

void suspend_wakeup_init_user(void) {
    status_suspended = false;
    status_invalidate_frame_cache();
    companion_invalidate_report_cache();
    status_render();
}

layer_state_t layer_state_set_user(layer_state_t state) {
    companion_queue_layer_state(state);
    return state;
}

bool led_update_user(led_t led_state) {
    status_caps_lock   = led_state.caps_lock;
    status_num_lock    = led_state.num_lock;
    status_scroll_lock = led_state.scroll_lock;
    return true;
}

static void trackball_timeout_task(void) {
    bool mode_changed = false;

    for (uint8_t side = 0; side < ARRAY_SIZE(trackball_states); side++) {
        trackball_state_t *state = &trackball_states[side];

        if (state->latched_mode != TB_MODE_CURSOR && state->activity_timer != 0 && timer_elapsed32(state->activity_timer) > TB_MODE_TIMEOUT_MS) {
            trackball_clear_side(side);
            mode_changed = true;
        } else if (state->axis_lock != TB_AXIS_NONE && state->activity_timer != 0 && timer_elapsed32(state->activity_timer) > TB_AXIS_TIMEOUT_MS) {
            state->axis_lock = TB_AXIS_NONE;
            state->v_accum   = 0;
            state->h_accum   = 0;
            state->key_accum = 0;
        }
    }

    if (mode_changed) {
        trackball_apply_cpi();
    }
}

void housekeeping_task_user(void) {
    trackball_timeout_task();
    status_usb_recovery_task();
    companion_flush_pending_layer_state();
    status_render();
    companion_task();
}

void raw_hid_receive(uint8_t *data, uint8_t length) {
    if (length != COMPANION_REPORT_SIZE) {
        return;
    }

    if (data[0] != COMPANION_MAGIC_0 || data[1] != COMPANION_MAGIC_1 || data[2] != COMPANION_PROTOCOL_VERSION) {
        return;
    }

    switch (data[3]) {
        case COMPANION_MSG_QUERY_STATUS:
            companion_query_status_pending = true;
            break;

        case COMPANION_MSG_QUERY_CLASS:
            if (data[5] < COMPANION_CLASS_COUNT) {
                companion_query_class = data[5];
                companion_query_class_pending = true;
            }
            break;

        case COMPANION_MSG_QUERY_CONFIG:
            companion_query_config_pending = true;
            break;

        case COMPANION_MSG_SET_CONFIG:
            companion_handle_set_config(data);
            break;
    }
}

void pmw33xx_report_user(uint8_t sensor, pmw33xx_report_t report) {
    if (sensor != 0) {
        return;
    }

    companion_local_sensor_state.flags = COMPANION_SENSOR_VALID | (report.motion.b.is_lifted ? COMPANION_SENSOR_LIFTED : 0);
    companion_local_sensor_state.motion = report.motion.w;
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
    trackball_apply_cpi();
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
    trackball_apply_cpi();
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
            trackball_scroll_speed_up(TB_TARGET_BOTH);
            return false;

        case TB_SCRL_DOWN:
            trackball_scroll_speed_down(TB_TARGET_BOTH);
            return false;

        case TB_SCRL_DFLT:
            trackball_scroll_speed_default(TB_TARGET_BOTH);
            return false;

        case TB_L_SCRL_UP:
            trackball_scroll_speed_up(TB_TARGET_LEFT);
            return false;

        case TB_L_SCRL_DOWN:
            trackball_scroll_speed_down(TB_TARGET_LEFT);
            return false;

        case TB_L_SCRL_DFLT:
            trackball_scroll_speed_default(TB_TARGET_LEFT);
            return false;

        case TB_R_SCRL_UP:
            trackball_scroll_speed_up(TB_TARGET_RIGHT);
            return false;

        case TB_R_SCRL_DOWN:
            trackball_scroll_speed_down(TB_TARGET_RIGHT);
            return false;

        case TB_R_SCRL_DFLT:
            trackball_scroll_speed_default(TB_TARGET_RIGHT);
            return false;

        case TB_CPI_UP:
            trackball_cpi_up(TB_TARGET_BOTH);
            return false;

        case TB_CPI_DOWN:
            trackball_cpi_down(TB_TARGET_BOTH);
            return false;

        case TB_CPI_DFLT:
            trackball_cpi_default(TB_TARGET_BOTH);
            return false;

        case TB_L_CPI_UP:
            trackball_cpi_up(TB_TARGET_LEFT);
            return false;

        case TB_L_CPI_DOWN:
            trackball_cpi_down(TB_TARGET_LEFT);
            return false;

        case TB_L_CPI_DFLT:
            trackball_cpi_default(TB_TARGET_LEFT);
            return false;

        case TB_R_CPI_UP:
            trackball_cpi_up(TB_TARGET_RIGHT);
            return false;

        case TB_R_CPI_DOWN:
            trackball_cpi_down(TB_TARGET_RIGHT);
            return false;

        case TB_R_CPI_DFLT:
            trackball_cpi_default(TB_TARGET_RIGHT);
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
