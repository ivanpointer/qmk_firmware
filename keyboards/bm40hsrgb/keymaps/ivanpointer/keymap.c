#include QMK_KEYBOARD_H

#define _____ KC_NO
#define __VVV__ KC_TRNS

enum layers {
	_DVORAK,
	_SHIFT2,
	_NUMPAD,
	_ARROWS,
	_MEDIA,
	_FNKEYS,
	_SETTINGS
};

const uint16_t PROGMEM keymaps[][MATRIX_ROWS][MATRIX_COLS] = {
	[_DVORAK] = LAYOUT_planck_mit(
		KC_ESC, 		KC_SCLN, 		KC_COMM, 	KC_DOT, 		KC_P, 			KC_Y, 		KC_F, 		KC_G, 			KC_C, 			KC_R, 		KC_L, 		KC_BSPC,
		MO(_SHIFT2),	KC_A,	 		KC_O, 		KC_E, 			KC_U, 			KC_I, 		KC_D, 		KC_H, 			KC_T, 			KC_N, 		KC_S, 		MO(_SHIFT2),
		KC_LSFT, 		KC_QUOT, 		KC_Q, 		KC_J, 			KC_K, 			KC_X, 		KC_B, 		KC_M, 			KC_W, 			KC_V, 		KC_Z, 		KC_RSFT,
		KC_LCTL,		KC_LGUI, 		KC_LALT, 	TT(_NUMPAD), 	TT(_ARROWS), 		KC_SPC,				TT(_MEDIA), 	TT(_FNKEYS), 	KC_RALT, 	KC_RGUI, 	KC_RCTL),

	[_SHIFT2] = LAYOUT_planck_mit(
		TO(_DVORAK), 	_____, 			KC_LT, 		KC_GT, 			KC_DLR, 		KC_AMPR, 	KC_MINS, 	KC_SLSH, 		KC_BSLS, 		KC_AT, 		KC_HASH, 	KC_DEL,
		__VVV__, 		KC_LBRC, 		KC_LCBR, 	KC_RCBR, 		KC_LPRN, 		KC_EQL, 	KC_ASTR, 	KC_RPRN, 		KC_PLUS, 		KC_RBRC, 	KC_EXLM, 	__VVV__,
		__VVV__,		_____, 			_____, 		_____, 			KC_TILD, 		KC_PERC, 	KC_UNDS, 	KC_QUES, 		KC_PIPE, 		KC_CIRC, 	KC_GRV, 	__VVV__,
		__VVV__, 		__VVV__, 		__VVV__, 	_____, 			KC_TAB, 			KC_ENT, 			KC_TAB, 		__VVV__, 		_____,		__VVV__, 	__VVV__),

	[_NUMPAD] = LAYOUT_planck_mit(
		TO(_DVORAK), 	_____, 			KC_COPY, 	KC_PSTE, 		KC_CUT, 		_____, 		KC_PAST, 	KC_P7, 			KC_P8, 			KC_P9, 		KC_PMNS, 	KC_BSPC,
		_____, 			KC_LSFT, 		KC_LCTL, 	KC_LGUI, 		KC_LALT, 		_____, 		KC_PSLS, 	KC_P4, 			KC_P5, 			KC_P6, 		KC_PPLS, 	_____,
		__VVV__, 		_____, 			_____, 		_____, 			_____, 			_____, 		KC_NLCK, 	KC_P1, 			KC_P2, 			KC_P3, 		KC_PDOT, 	__VVV__,
		__VVV__, 		__VVV__, 		__VVV__, 	__VVV__, 		_____, 				KC_PENT,			KC_P0, 			KC_P0, 			__VVV__, 	__VVV__, 	__VVV__),

	[_ARROWS] = LAYOUT_planck_mit(
		TO(_DVORAK), 	_____, 			KC_COPY, 	KC_PSTE, 		KC_CUT, 		_____, 		_____, 		KC_HOME, 		KC_UP, 			KC_END, 	KC_PGUP, 	KC_BSPC,
		_____, 			KC_LSFT, 		KC_LCTL, 	KC_LGUI, 		KC_LALT, 		_____, 		_____, 		KC_LEFT, 		KC_DOWN, 		KC_RGHT, 	KC_PGDN, 	KC_DEL,
		__VVV__, 		_____, 			_____, 		_____, 			_____, 			_____, 		_____, 		_____, 			_____, 			_____, 		_____, 		__VVV__,
		__VVV__, 		__VVV__, 		__VVV__, 	_____, 			__VVV__, 			KC_ENT, 			KC_TAB, 		_____, 			__VVV__, 	__VVV__, 	__VVV__),

	[_MEDIA] = LAYOUT_planck_mit(
		TO(_DVORAK), 	_____,			_____, 		KC_MPRV, 		KC_MNXT, 		_____, 		_____, 		_____, 			_____,			KC_SLCK, 	KC_PAUS, 	KC_PSCR,
		KC_CAPS, 		_____, 			_____, 		KC_VOLD, 		KC_VOLU, 		_____, 		_____, 		_____, 			_____, 			_____, 		_____, 		KC_APP,
		__VVV__, 		_____, 			_____, 		KC_MPLY, 		KC_MUTE, 		_____, 		_____, 		_____, 			_____, 			_____, 		_____, 		__VVV__,
		__VVV__, 		__VVV__, 		__VVV__,	_____, 			_____, 				_____, 				_____, 			__VVV__, 		__VVV__, 	__VVV__, 	__VVV__),

	[_FNKEYS] = LAYOUT_planck_mit(
		TO(_DVORAK), 	KC_F1, 			KC_F2, 		KC_F3, 			KC_F4, 			_____, 		_____, 		KC_F13, 		KC_F14, 		KC_F15, 	KC_F16, 	TO(_SETTINGS),
		_____, 			KC_F5, 			KC_F6, 		KC_F7, 			KC_F8, 			_____, 		_____, 		KC_F17, 		KC_F18, 		KC_F19, 	KC_F20, 	_____,
		__VVV__, 		KC_F9, 			KC_F10, 	KC_F11, 		KC_F12, 		_____, 		_____, 		KC_F21, 		KC_F22, 		KC_F23, 	KC_F24, 	__VVV__,
		__VVV__, 		__VVV__, 		__VVV__, 	_____, 			_____, 				_____, 				__VVV__, 		_____, 			__VVV__, 	__VVV__, 	__VVV__),

	[_SETTINGS] = LAYOUT_planck_mit(
		TO(_DVORAK),	_____,	 		_____, 		_____, 			_____, 			_____, 		_____,		_____, 			_____, 			_____,	 	_____, 		RESET,
		_____, 			_____, 			_____, 		_____, 			_____, 			_____, 		_____, 		_____,			_____, 			_____,	 	_____, 		RGB_TOG,
		_____, 			_____, 			_____, 		_____, 			_____, 			_____, 		_____, 		_____, 			_____, 			_____, 		_____, 		_____,
		_____, 			_____, 			_____, 		_____, 			_____, 				_____, 				_____, 			_____, 			_____, 		_____, 		_____)
};

/*            HSV COLORS
#define HSV_WHITE 0, 0, 255
#define HSV_RED 0, 255, 255
#define HSV_CORAL 11, 176, 255
#define HSV_ORANGE 28, 255, 255
#define HSV_GOLDENROD 30, 218, 218
#define HSV_GOLD 36, 255, 255
#define HSV_YELLOW 43, 255, 255
#define HSV_CHARTREUSE 64, 255, 255
#define HSV_GREEN 85, 255, 255
#define HSV_SPRINGGREEN 106, 255, 255
#define HSV_TURQUOISE 123, 90, 112
#define HSV_TEAL 128, 255, 128
#define HSV_CYAN 128, 255, 255
#define HSV_AZURE 132, 102, 255
#define HSV_BLUE 170, 255, 255
#define HSV_PURPLE 191, 255, 255
#define HSV_MAGENTA 213, 255, 255
#define HSV_PINK 234, 128, 255
#define HSV_BLACK 0, 0, 0
#define HSV_OFF HSV_BLACK
            */

#define LAYER_IS_ON(layer_state, layer_num) ((layer_state & (1 << layer_num)) > 0)

#define PAL_1 HSV_BLUE
#define PAL_2 HSV_RED
#define PAL_3 HSV_GREEN
#define PAL_4 HSV_PURPLE
#define PAL_5 HSV_CYAN

#define PAL_BASE HSV_BLUE
#define PAL_SYMBOLS HSV_GREEN
#define PAL_DANGER HSV_RED
#define PAL_CNTRL HSV_PURPLE
#define PAL_CNTRL_2 HSV_YELLOW
#define PAL_CNTRL_3 HSV_CYAN

#define PAL_LAYERS PAL_CNTRL_3

const rgblight_segment_t PROGMEM my_dvorak_layer[] = RGBLIGHT_LAYER_SEGMENTS(
	{0, 47, PAL_BASE }, // Base color

	{1, 3, PAL_SYMBOLS}, // ; , .
	{25, 1, PAL_SYMBOLS}, // '

	{0, 1, PAL_DANGER }, // Esc
	{11, 1, PAL_DANGER }, // Back-Space
	{12, 1, PAL_CNTRL }, // Left Shift-2
	{23, 1, PAL_CNTRL }, // Right Shift-2
	{24, 1, PAL_CNTRL }, // Left Shift
	{35, 1, PAL_CNTRL }, // Right Shift
	{36, 3, PAL_CNTRL }, // Left controls
	{39, 2, PAL_LAYERS }, // Left layer-shifts
	{42, 2, PAL_LAYERS }, // Right layer-shifts
	{44, 3, PAL_CNTRL }, // Right controls
	{47, 6, HSV_OFF } // Glow Lights
);

const rgblight_segment_t PROGMEM my_shift_layer[] = RGBLIGHT_LAYER_SEGMENTS(
	{0, 47, HSV_OFF }, // Base color

	{2, 9, PAL_SYMBOLS }, // Row 1
	{13, 10, PAL_SYMBOLS }, // Row 2
	{28, 7, PAL_SYMBOLS }, // Row 3

	{0, 1, PAL_LAYERS }, // TO 0
	{11, 1, PAL_DANGER }, // Delete
	{12, 1, PAL_CNTRL }, // Left Shift-2
	{23, 1, PAL_CNTRL }, // Right Shift-2
	{24, 1, PAL_CNTRL }, // Left Shift
	{35, 1, PAL_CNTRL }, // Right Shift
	{36, 3, PAL_CNTRL }, // Left controls
	{40, 1, PAL_CNTRL_2 }, // Left Tab
	{41, 1, PAL_CNTRL_2 }, // Enter
	{42, 1, PAL_CNTRL_2 }, // Right Tab
	{44, 3, PAL_CNTRL }, // Right contrils
	{47, 6, HSV_OFF } // Glow Lights
);


const rgblight_segment_t PROGMEM my_numpad_layer[] = RGBLIGHT_LAYER_SEGMENTS(
    {0, 47, HSV_OFF }, // Base color
	{0, 1, PAL_LAYERS }, // TO 0

	{2, 2, PAL_CNTRL_2 }, // Copy, Paste
	{4, 1, PAL_DANGER }, // Cut
	{13, 4, PAL_CNTRL }, // Shift-Alt
	{11, 1, PAL_DANGER }, // Back Space

	{ 7, 3, PAL_CNTRL_2 }, // 7-9
	{ 19, 3, PAL_CNTRL_2 }, // 4-6
	{ 31, 3, PAL_CNTRL_2 }, // 1-3
	{ 42, 2, PAL_CNTRL_2 }, // 0

	{ 6, 1, PAL_SYMBOLS}, // *
	{ 18, 1, PAL_SYMBOLS}, // /
	{ 10, 1, PAL_SYMBOLS}, // -
	{ 22, 1, PAL_SYMBOLS}, // +
	{ 34, 1, PAL_SYMBOLS}, // *

    { 30, 1, PAL_CNTRL }, // Num Lock
	{ 41, 1, PAL_CNTRL }, // Enter

    { 24, 1, PAL_CNTRL }, // Left Shift
	{ 35, 1, PAL_CNTRL }, // Right Shift
	{ 36, 3, PAL_CNTRL }, // Left controls
	{ 44, 3, PAL_CNTRL }, // Right controls

	{ 47, 6, HSV_OFF } // Glow Lights
);

const rgblight_segment_t PROGMEM my_arrows_layer[] = RGBLIGHT_LAYER_SEGMENTS(
	{0, 47, HSV_OFF }, // Base color
	{0, 1, PAL_LAYERS }, // TO 0
	{2, 1, PAL_CNTRL_2 }, // Copy
	{3, 1, 	PAL_CNTRL_2 }, // Paste
	{4, 1, PAL_DANGER }, // Cut
	{7, 1, PAL_CNTRL_2 }, // Home
	{8, 1, PAL_CNTRL }, // Up
	{9, 1, PAL_CNTRL_2 }, // End
	{10, 1, PAL_CNTRL_2 }, // Pg Up
	{11, 1, PAL_DANGER }, // Back Space

	{13, 4, PAL_CNTRL }, // Shift-Alt
	{19, 3, PAL_CNTRL }, // Left, Down, Right
	{22, 2, PAL_CNTRL_2 }, // Pg Dwn

    {24, 1, PAL_CNTRL }, // Left Shift
	{35, 1, PAL_CNTRL }, // Right Shift
	{36, 3, PAL_CNTRL }, // Left controls

	{41, 1, PAL_CNTRL }, // Enter
	{42, 1, PAL_CNTRL_2 }, // Right Tab

	{44, 3, PAL_CNTRL }, // Right controls
	{47, 6, HSV_OFF } // Glow Lights
);

const rgblight_segment_t PROGMEM my_media_layer[] = RGBLIGHT_LAYER_SEGMENTS(
	{0, 47, HSV_OFF }, // Base color
	{0, 1, PAL_LAYERS }, // TO 0

	{ 9, 3, PAL_CNTRL_2 }, // Scroll Lock, Print Screen
	{ 12, 1, PAL_CNTRL_2 }, // Caps
	{ 13, 1, PAL_CNTRL_2 }, // Menu

	{ 3, 2, PAL_CNTRL }, // Prev, Next
	{ 15, 2, PAL_CNTRL }, // Vol - +
	{ 27, 2, PAL_CNTRL }, // Play, Mute

    {24, 1, PAL_CNTRL }, // Left Shift
	{35, 1, PAL_CNTRL }, // Right Shift
	{36, 3, PAL_CNTRL }, // Left controls
	{44, 3, PAL_CNTRL }, // Right controls

	{47, 6, HSV_OFF } // Glow Lights
);

const rgblight_segment_t PROGMEM my_fnkeys_layer[] = RGBLIGHT_LAYER_SEGMENTS(
	{0, 47, HSV_OFF }, // Base color
	{0, 1, PAL_LAYERS }, // TO 0
	{11, 1, PAL_DANGER }, // TO 6

	{1, 4, PAL_1 }, // F1-F4
	{13, 4, PAL_5 }, // F5-F8
	{25, 4, PAL_3 }, // F9-F12

	{7, 4, PAL_1 }, // F13-F16
	{19, 4, PAL_5 }, // F17-F20
	{31, 4, PAL_3 }, // F21-F24

	{24, 1, PAL_CNTRL }, // Left Shift
	{35, 1, PAL_CNTRL }, // Right Shift
	{36, 3, PAL_CNTRL }, // Left controls
	{44, 3, PAL_CNTRL }, // Right controls

	{47, 6, HSV_OFF } // Glow Lights
);

const rgblight_segment_t PROGMEM my_settings_layer[] = RGBLIGHT_LAYER_SEGMENTS(
	{0, 47, HSV_OFF }, // Base color

	{0, 1, PAL_LAYERS }, // TO 0

	{ 11, 1, PAL_DANGER }, // Reset

	{ 23, 1, PAL_CNTRL_3 }, // RGB Toggle

	{47, 6, PAL_DANGER } // Glow Lights
);

// Now define the array of layers. Later layers take precedence
const rgblight_segment_t* const PROGMEM my_rgb_layers[] = RGBLIGHT_LAYERS_LIST(
    my_dvorak_layer,
	my_shift_layer,
	my_numpad_layer,
	my_arrows_layer,
	my_media_layer,
	my_fnkeys_layer,
	my_settings_layer
);

void keyboard_post_init_user(void) {
    // Enable the LED layers
    rgblight_layers = my_rgb_layers;

	// Start with the base layer
	rgblight_set_layer_state(_DVORAK, true);
}

uint32_t layer_state_set_user(uint32_t state) {
	// Overwrite the base with whatever layers are active
	rgblight_set_layer_state(_SHIFT2, LAYER_IS_ON(state, _SHIFT2));
	rgblight_set_layer_state(_NUMPAD, LAYER_IS_ON(state, _NUMPAD));
	rgblight_set_layer_state(_ARROWS, LAYER_IS_ON(state, _ARROWS));
	rgblight_set_layer_state(_MEDIA, LAYER_IS_ON(state, _MEDIA));
	rgblight_set_layer_state(_FNKEYS, LAYER_IS_ON(state, _FNKEYS));
	rgblight_set_layer_state(_SETTINGS, LAYER_IS_ON(state, _SETTINGS));

	// Continue the chain
	return state;
}

bool led_update_user(led_t led_state) {
	// rgblight_set_layer_state(_DVORAK, led_state.caps_lock);
	return true;
}

layer_state_t default_layer_state_set_user(layer_state_t state) {
	// Set back to the base layer
    rgblight_set_layer_state(_DVORAK, true);
    return state;
}
