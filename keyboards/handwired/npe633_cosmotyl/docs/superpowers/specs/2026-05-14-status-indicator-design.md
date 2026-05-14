# Status Indicator Design

## Goal

Add status feedback to the YD-RP2040 onboard RGB LED on GP23 for the `handwired/npe633_cosmotyl` keymap.

## Hardware And QMK Feature

The status indicator uses the controller's single onboard WS2812 RGB LED. QMK drives it through RGBLight, with the WS2812 data pin configured as GP23 and the RP2040 vendor WS2812 driver selected.

The feature is local to each half. Each half renders its own onboard LED based on the status that firmware can observe locally.

## Status Priority

The indicator renders exactly one status at a time, in this priority order:

1. Startup flash
2. Error blink code
3. Caps Lock breathing
4. Current layer color

Higher-priority states override lower-priority states. When a higher-priority state ends, rendering falls back to the next active state.

## Normal Layer Colors

The normal idle state is a static color for the highest active layer. Layer changes update the color immediately.

Normal layer colors avoid red, orange, amber, pink, and magenta so error states remain visually distinct. The palette uses saturated colors and simple non-red alternating patterns because the LED is transported through a fiber pipe to a small chassis hole.

- `_BASE`: solid white
- `_SH2`: alternating white / blue
- `_MOU`: solid cyan
- `_NAV`: solid green
- `_NUM`: solid yellow
- `_FN1`: solid purple
- `_FN2`: alternating purple / cyan
- `_SYS`: alternating purple / yellow

The palette lives in the keymap so it can be adjusted while iterating on the layout.

## Startup Flash

On boot, the LED flashes briefly before normal status rendering starts. The flash is visually obvious without delaying keyboard startup.

The startup effect is a short white flash sequence lasting about one second.

## Caps Lock

When Caps Lock is active, the LED smooth-breathes instead of showing the normal layer indicator. The breathing color uses the current layer's active color so Caps Lock remains visible without hiding which layer is active. Caps Lock is the only smooth breathing status; layer alternation hard-switches between colors instead.

When Caps Lock turns off, rendering returns to the current layer color unless an error is active.

## Error Blink Codes

Errors override both Caps Lock and layer colors. Errors are shown as repeating red blink codes:

- 1 red blink: split transport is expected but disconnected
- 2 red blinks: pointing device status is not success

If more than one error is active, the indicator shows the highest-priority error code. Split transport has priority over pointing device status because it affects the whole keyboard's coordination.

Error detection avoids false positives during the startup flash window. After startup, the status task checks errors periodically and updates the active code. Persistent errors display for 30 seconds, then fall back to the normal layer or Caps Lock status until the error changes, clears, or reappears after clearing.

## Controls

The existing `_SYS` placeholders become status controls:

- `STAT_TOGG`: toggle status indicator on or off
- `STAT_BRID`: reduce status brightness
- `STAT_BRIU`: increase status brightness
- `STAT_TEST`: trigger the startup flash pattern for visual testing

Controls affect only the status indicator behavior and do not write to EEPROM during normal status updates.

## Implementation Shape

Keep the first implementation inside `keymaps/default/keymap.c` plus keyboard configuration in `rules.mk` and `keyboard.json`.

Use `keyboard_post_init_user()` to initialize RGBLight and start the startup flash. Use `layer_state_set_user()` and `led_update_user()` to mark visible state changes. Use `housekeeping_task_user()` to render the current status pattern on a timer.

Use QMK helpers such as `get_highest_layer()`, `host_keyboard_led_state()`, `pointing_device_get_status()`, and `is_transport_connected()` where available.

## Verification

The implementation is verified by compiling the default keymap:

```bash
qmk compile -kb handwired/npe633_cosmotyl -km default
```

Hardware verification after flashing checks:

- Startup flashes briefly.
- Each layer displays a distinct static color.
- Caps Lock breathes using the current layer color.
- Disconnecting the split side produces a one-blink red code.
- A pointing sensor failure produces a two-blink red code.
- `_SYS` status controls toggle, dim, brighten, and replay the startup flash.
