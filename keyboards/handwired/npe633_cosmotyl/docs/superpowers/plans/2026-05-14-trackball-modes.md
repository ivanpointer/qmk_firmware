# Trackball Modes Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add source-targeted trackball modes for the NPE633 Cosmotyl, including smart scroll, pan, volume, brightness, zoom, rotate, and a base reset that clears trackball state.

**Architecture:** Keep left and right pointing reports separate through calibration and mode handling, then combine only the remaining cursor/wheel reports. Trackball mode keys set per-side held or latched roles, and `TB_BASE` clears layers plus all firmware-side trackball state. Status rendering remains single-LED for now, but trackball and lock state are exposed through a status model so later multi-LED output can be added cleanly.

**Tech Stack:** QMK firmware, PMW3389 pointing device, split combined pointing callbacks, custom keycodes in `keymaps/default/keymap.c`, existing `qmk compile -kb handwired/npe633_cosmotyl -km default` verification.

---

## File Structure

- Modify `keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c`
  - Add custom trackball keycodes.
  - Add per-side trackball role state.
  - Add motion-to-scroll/key/macro processing.
  - Add `TB_BASE` reset behavior.
  - Refactor status rendering to consume a small status model while still rendering to one LED.
  - Add Num Lock status behavior alongside existing Caps Lock behavior.
- Modify `keyboards/handwired/npe633_cosmotyl/config.h`
  - Add local timing and threshold constants only if they are better kept with other board configuration. Prefer `keymap.c` for first-pass constants so the feature stays keymap-local.

## Manual Behavior Targets

- Right ball remains normal cursor by default.
- Left ball can be set to smart scroll without holding a key; right ball continues cursor movement while left ball scrolls.
- Held mode keys are momentary.
- Tapped mode keys latch a mode until inactivity timeout or `TB_BASE`.
- Smart scroll locks to vertical or horizontal based on first meaningful movement, then unlocks after motion timeout.
- `TB_BASE` returns to base layer and clears trackball modes, axis locks, and accumulators.
- Caps Lock and Num Lock remain host lock states and are not changed by `TB_BASE`.
- Single LED remains the only physical status output in this pass.

---

### Task 1: Add Trackball Mode Types and Constants

**Files:**
- Modify: `keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c`

- [ ] **Step 1: Extend custom keycodes**

Replace the current custom keycode enum:

```c
enum custom_keycodes {
    STAT_TOGG = SAFE_RANGE,
    STAT_BRID,
    STAT_BRIU,
    STAT_TEST,
};
```

with:

```c
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
```

- [ ] **Step 2: Replace current trackball aliases**

Replace:

```c
// Placeholders for DPI adjustments for trackball
#define CPI_UP   KC_NO
#define CPI_DOWN KC_NO
#define CPI_DFLT KC_NO

// Trackball Scroll-Mode placeholder
#define SCRL_MOD KC_NO
```

with:

```c
// Trackball mode aliases used in the layout below.
#define SCRL_MOD TB_L_SMRT
#define CPI_UP   TB_L_BRI
#define CPI_DOWN TB_L_VOL
#define CPI_DFLT TB_L_PAN
```

- [ ] **Step 3: Add mode enums and constants after `enum status_error_code`**

Insert:

```c
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

#define TB_MODE_TIMEOUT_MS 1200
#define TB_AXIS_TIMEOUT_MS 600
#define TB_AXIS_LOCK_THRESHOLD 4
#define TB_AXIS_LOCK_RATIO 2
#define TB_SCROLL_DIVISOR 8
#define TB_KEY_THRESHOLD 24
#define TB_ROTATE_THRESHOLD 32
```

- [ ] **Step 4: Add per-side state near the existing status globals**

After:

```c
static status_hsv_t status_last_hsv = {HSV_OFF};
```

insert:

```c
static trackball_state_t trackball_states[] __attribute__((unused)) = {
    [TB_SIDE_LEFT]  = {0},
    [TB_SIDE_RIGHT] = {0},
};

static void status_render(void);
```

- [ ] **Step 5: Compile**

Run:

```bash
qmk compile -kb handwired/npe633_cosmotyl -km default
```

Expected: compile succeeds and emits `handwired_npe633_cosmotyl_default.uf2`.

- [ ] **Step 6: Commit**

Run:

```bash
git add keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c
git commit -m "Add NPE633 trackball mode keycodes"
```

---

### Task 2: Add Trackball State Helpers and Base Reset

**Files:**
- Modify: `keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c`

- [ ] **Step 1: Add helper functions before `pointing_device_task_combined_user()`**

Insert this block after `rotate_left_pointing_report()` and before `pointing_device_task_combined_user()`:

```c
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
```

- [ ] **Step 2: Initialize trackball state on startup**

In `keyboard_post_init_user()`, after:

```c
status_last_hsv          = (status_hsv_t){HSV_OFF};
```

add:

```c
trackball_clear_all();
```

- [ ] **Step 3: Wire `TB_BASE` in `process_record_user()`**

At the top of the `switch (keycode)` in `process_record_user()`, before `STAT_TOGG`, insert:

```c
        case TB_BASE:
            reset_keyboard_state_to_base();
            return false;
```

- [ ] **Step 4: Replace base-return keys**

Replace the fourteen current `TO(_BASE)` entries in these rows with `TB_BASE`:

```text
_SH2 top row: both center base-return positions
_MOU top row: both center base-return positions
_NAV top row: both center base-return positions
_NUM top row: both center base-return positions
_FN1 top row: both center base-return positions
_FN2 top row: both center base-return positions
_SYS top row: both center base-return positions
```

- [ ] **Step 5: Compile**

Run:

```bash
qmk compile -kb handwired/npe633_cosmotyl -km default
```

Expected: compile succeeds.

- [ ] **Step 6: Manual hardware check**

Flash the UF2 and verify:

```text
1. Enter a non-base layer using an existing toggle/layer key.
2. Press one of the base-return keys.
3. Keyboard returns to base behavior.
4. Caps Lock state remains unchanged if it was enabled.
```

- [ ] **Step 7: Commit**

Run:

```bash
git add keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c
git commit -m "Reset trackball modes from base key"
```

---

### Task 3: Add Mode Key Press, Hold, and Latch Handling

**Files:**
- Modify: `keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c`

- [ ] **Step 1: Add mode key lookup helpers before `process_record_user()`**

Insert:

```c
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
        state->latched_mode = state->held_mode;
        state->activity_timer = timer_read32();
    }

    state->held_mode        = TB_MODE_CURSOR;
    state->axis_lock        = TB_AXIS_NONE;
    state->moved_while_held = false;
    state->v_accum          = 0;
    state->h_accum          = 0;
    state->key_accum        = 0;
}
```

- [ ] **Step 2: Update `process_record_user()` release handling**

Replace:

```c
    if (!record->event.pressed) {
        return true;
    }
```

with:

```c
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
```

- [ ] **Step 3: Add timeout clearing in `housekeeping_task_user()`**

Replace:

```c
void housekeeping_task_user(void) {
    status_render();
}
```

with:

```c
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
```

- [ ] **Step 4: Compile**

Run:

```bash
qmk compile -kb handwired/npe633_cosmotyl -km default
```

Expected: compile succeeds.

- [ ] **Step 5: Commit**

Run:

```bash
git add keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c
git commit -m "Add held and latched trackball modes"
```

---

### Task 4: Process Scroll, Pan, Volume, Brightness, Zoom, and Rotate

**Files:**
- Modify: `keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c`

- [ ] **Step 1: Add motion helpers before `pointing_device_task_combined_user()`**

Insert after the Task 2 trackball state helpers and before `pointing_device_task_combined_user()`:

```c
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
```

- [ ] **Step 2: Add per-side mode processor before `pointing_device_task_combined_user()`**

Insert:

```c
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
```

- [ ] **Step 3: Update the combined pointing callback**

Replace:

```c
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
    left_report = rotate_left_pointing_report(left_report);

    return pointing_device_combine_reports(left_report, right_report);
}
```

with:

```c
report_mouse_t pointing_device_task_combined_user(report_mouse_t left_report, report_mouse_t right_report) {
    left_report  = rotate_left_pointing_report(left_report);
    left_report  = trackball_process_side(TB_SIDE_LEFT, left_report);
    right_report = trackball_process_side(TB_SIDE_RIGHT, right_report);

    return pointing_device_combine_reports(left_report, right_report);
}
```

- [ ] **Step 4: Compile**

Run:

```bash
qmk compile -kb handwired/npe633_cosmotyl -km default
```

Expected: compile succeeds.

- [ ] **Step 5: Manual hardware check**

Flash and verify:

```text
1. Hold SCRL_MOD and move left ball mostly up/down: host scrolls vertically.
2. Hold SCRL_MOD and move left ball mostly left/right: host scrolls horizontally.
3. Tap SCRL_MOD, release it, then move left ball: mode latches.
4. While left ball is latched to smart scroll, right ball still moves cursor.
5. Wait more than TB_MODE_TIMEOUT_MS without moving left ball: left ball returns to cursor.
6. Use CPI_UP/CPI_DOWN/CPI_DFLT aliases on the mouse layer to check brightness, volume, and pan.
```

- [ ] **Step 6: Commit**

Run:

```bash
git add keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c
git commit -m "Process NPE633 trackball modes"
```

---

### Task 5: Map Remaining Mode Keys on the Mouse Layer

**Files:**
- Modify: `keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c`

- [ ] **Step 1: Add explicit aliases for remaining modes near the existing aliases**

Insert after the `CPI_DFLT` alias:

```c
#define TB_L_ZOOM_KEY TB_L_ZOOM
#define TB_L_ROT_KEY  TB_L_ROT
#define TB_R_SMRT_KEY TB_R_SMRT
#define TB_R_PAN_KEY  TB_R_PAN
```

- [ ] **Step 2: Update the fourth row of `_MOU` right-hand mode cluster**

Replace this `_MOU` row:

```c
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       SCRL_MOD , CPI_DFLT , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_TRNS,
```

with:

```c
        KC_TRNS  , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    , KC_NO    ,                       SCRL_MOD , CPI_DFLT , TB_L_ZOOM_KEY, TB_L_ROT_KEY, TB_R_SMRT_KEY, TB_R_PAN_KEY, KC_TRNS,
```

- [ ] **Step 3: Compile**

Run:

```bash
qmk compile -kb handwired/npe633_cosmotyl -km default
```

Expected: compile succeeds. Do not run `qmk-keymap-format.py` in this task; preserve the existing hand-aligned layout style.

- [ ] **Step 4: Manual hardware check**

Flash and verify:

```text
1. Mouse layer exposes left zoom and rotate keys.
2. Mouse layer exposes right smart-scroll and right pan keys.
3. No regular typing layer gained an accidental mode key except SCRL_MOD.
```

- [ ] **Step 5: Commit**

Run:

```bash
git add keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c
git commit -m "Map NPE633 trackball mode keys"
```

---

### Task 6: Refactor Single-LED Status Around a Status Model

**Files:**
- Modify: `keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c`

- [ ] **Step 1: Add status model type after `status_layer_style_t`**

Insert:

```c
typedef struct {
    uint8_t             error_code;
    uint8_t             active_layer;
    bool                startup_active;
    bool                caps_lock;
    bool                num_lock;
    enum trackball_mode left_mode;
    enum trackball_mode right_mode;
} status_model_t;
```

- [ ] **Step 2: Add model collection helper before `status_render()`**

Insert:

```c
static status_model_t status_collect_model(uint32_t now) {
    led_t led_state = host_keyboard_led_state();
    uint8_t layer   = get_highest_layer(layer_state | default_layer_state);

    if (layer >= ARRAY_SIZE(status_layer_styles)) {
        layer = _BASE;
    }

    return (status_model_t){
        .error_code     = status_error_code,
        .active_layer   = layer,
        .startup_active = timer_elapsed32(status_startup_timer) < STATUS_STARTUP_MS,
        .caps_lock      = led_state.caps_lock,
        .num_lock       = led_state.num_lock,
        .left_mode      = trackball_effective_mode(TB_SIDE_LEFT),
        .right_mode     = trackball_effective_mode(TB_SIDE_RIGHT),
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
```

- [ ] **Step 3: Update normal status rendering**

Delete the existing `status_style_for_active_layer()` function, because `status_collect_model()` will own active-layer selection after this task.

Replace `status_render_normal(uint32_t now)` with:

```c
static void status_render_normal(uint32_t now, status_model_t model) {
    status_layer_style_t style = status_layer_styles[model.active_layer];
    status_hsv_t         color = style.primary;

    if (model.left_mode != TB_MODE_CURSOR) {
        color = status_color_for_trackball_mode(model.left_mode);
    } else if (model.right_mode != TB_MODE_CURSOR) {
        color = status_color_for_trackball_mode(model.right_mode);
    } else if (style.alternate && ((now / STATUS_LAYER_ALTERNATE_MS) % 2) == 1) {
        color = style.secondary;
    }

    if (model.num_lock && model.left_mode == TB_MODE_CURSOR && model.right_mode == TB_MODE_CURSOR && ((now / STATUS_LAYER_ALTERNATE_MS) % 2) == 1) {
        color = (status_hsv_t){HSV_WHITE};
    }

    if (model.caps_lock) {
        color.v = status_wave_value(now, STATUS_BREATHE_MS, STATUS_BREATHE_MIN);
    } else {
        color.v = status_brightness;
    }

    status_set_hsv(color);
}
```

- [ ] **Step 4: Update `status_render()` to pass the model**

Replace:

```c
    if (timer_elapsed32(status_startup_timer) < STATUS_STARTUP_MS) {
        status_render_startup();
        return;
    }
```

with:

```c
    status_model_t model = status_collect_model(now);

    if (model.startup_active) {
        status_render_startup();
        return;
    }
```

Then replace:

```c
    status_render_normal(now);
```

with:

```c
    status_render_normal(now, model);
```

- [ ] **Step 5: Compile**

Run:

```bash
qmk compile -kb handwired/npe633_cosmotyl -km default
```

Expected: compile succeeds.

- [ ] **Step 6: Manual hardware check**

Flash and verify:

```text
1. Startup animation still appears.
2. Layer colors still appear when no trackball mode is active.
3. Active trackball mode changes the single LED color.
4. Caps Lock still breathes brightness.
5. Num Lock alternates the normal color with white when no trackball mode is active.
6. Split and pointing error indication still overrides normal display.
```

- [ ] **Step 7: Commit**

Run:

```bash
git add keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c
git commit -m "Model NPE633 status for trackball modes"
```

---

### Task 7: Final Verification and Push

**Files:**
- Verify: `keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c`
- Verify: `keyboards/handwired/npe633_cosmotyl/config.h`

- [ ] **Step 1: Inspect final diff**

Run:

```bash
git diff origin/master..HEAD -- keyboards/handwired/npe633_cosmotyl/config.h keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c
```

Expected: diff contains only NPE633 trackball mode, base reset, and status model work.

- [ ] **Step 2: Run whitespace check**

Run:

```bash
git diff --check origin/master..HEAD -- keyboards/handwired/npe633_cosmotyl/config.h keyboards/handwired/npe633_cosmotyl/keymaps/default/keymap.c
```

Expected: no output and exit code 0.

- [ ] **Step 3: Run final compile**

Run:

```bash
qmk compile -kb handwired/npe633_cosmotyl -km default
```

Expected: compile succeeds and emits `handwired_npe633_cosmotyl_default.uf2`.

- [ ] **Step 4: Manual acceptance test**

Flash and verify:

```text
1. Default cursor movement still works on both balls.
2. Left and right calibration still feel close.
3. Hold SCRL_MOD: left ball smart-scrolls; right ball still moves cursor.
4. Tap SCRL_MOD: left smart-scroll latches; timeout returns it to cursor.
5. TB_BASE clears latched trackball mode and returns to base layer.
6. Pan sends horizontal and vertical wheel motion.
7. Volume and brightness emit the expected OS controls.
8. Zoom emits Cmd+Plus/Cmd+Minus.
9. Rotate emits Alt+Bracket shortcuts for app-level binding.
10. Caps Lock and Num Lock status behavior is visible.
```

- [ ] **Step 5: Push**

Run:

```bash
git status --short --branch
git push
```

Expected: local `master` pushes to `git@github.com:ivanpointer/qmk_firmware.git`.

---

## Self-Review Notes

- The plan deliberately excludes knob support.
- The plan keeps multi-LED hardware out of scope.
- The status model introduced in Task 6 is the future extension point for multi-LED rendering.
- `TB_BASE` is the reset mechanism for firmware-side state; it does not change host lock states.
- Pan is included as direct two-axis wheel output.
- Desktop gestures and Mission Control are not implemented in this first pass, but the custom keycode and mode architecture leaves room for dedicated gesture modes later.
