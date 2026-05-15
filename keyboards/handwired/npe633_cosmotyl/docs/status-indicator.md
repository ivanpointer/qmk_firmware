# Status Indicator

Each half has a four-LED RGBLight status string on GP23. The LEDs are intended to feed four separate fiber strands.

## Normal Roles

| Light | Role | Behavior |
| --- | --- | --- |
| 1 | Pointing state | CPI non-default or precision mode |
| 2 | Trackball mode | This half's current trackball mode |
| 3 | Layer | Current active layer color |
| 4 | Board status and locks | Error, sleep, disconnected, Caps Lock, Num Lock, and Scroll Lock |

The master half renders both halves and sends the remote half's four-light frame over a split RPC transaction. This keeps the trackball mode indicator half-specific even when the left and right balls are in different modes.

## Board Status And Locks

| State | Indicator |
| --- | --- |
| No lock / no error | Light 4 is white |
| Startup | All four lights flash white |
| USB suspended / host asleep | Light 4 breathes blue |
| USB not configured / no host | Light 4 breathes magenta |
| Split transport disconnected | Light 4 blinks one red pulse |
| Pointing device error | Light 4 blinks two red pulses |
| Caps Lock | Light 4 is green |
| Num Lock | Light 4 is orange |
| Scroll Lock | Light 4 is cyan |
| Multiple locks | Light 4 rotates through the active lock colors |

Error codes repeat until the error clears. Only the main status light is taken over by the error code; layer, trackball mode, and pointing status continue to render on their dedicated lights.

## Layer Indicators

| Layer | Indicator |
| --- | --- |
| `_BASE` | Dim white |
| `_SH2` | Full-brightness alternating white / blue |
| `_MOU` | Yellow |
| `_NAV` | Green |
| `_NUM` | Blue |
| `_FN1` | Purple |
| `_FN2` | Alternating purple / cyan |
| `_SYS` | Alternating purple / yellow |

## Trackball Mode

Light 2 is white when the local half's ball is in normal cursor mode.

| Mode | Indicator |
| --- | --- |
| Scroll / pan | Cyan |
| Volume | Green |
| Brightness | Yellow |
| Zoom | Blue |
| Rotate | Purple |

## Pointing State

Light 1 uses the same blue-to-orange gradient as the level overlay.

| State | Indicator |
| --- | --- |
| Precision mode held | White breathing |
| CPI level 1 | Blue |
| CPI level 2 | Soft blue-cyan |
| CPI level 3 | Cyan |
| CPI level 4 | Green |
| CPI level 5 | Yellow |
| CPI level 6 | Amber |
| CPI level 7 | Orange |

## Level Overlay

When adjusting scroll speed or CPI, all four lights temporarily show the selected seven-level setting. The selected level color graduates from blue at the lowest level to orange at the highest level.

| Level | Indicator |
| --- | --- |
| 1 | Light 1 |
| 2 | Lights 1 and 2 |
| 3 | Light 2 |
| 4 | Lights 2 and 3 |
| 5 | Light 3 |
| 6 | Lights 3 and 4 |
| 7 | Light 4 |

Level 4 is the default middle level for both scroll speed and CPI.

## Console

`qmk console` requires firmware compiled with `CONSOLE_ENABLE = yes`. This keyboard enables console in `rules.mk` so the device can be found by `qmk console` after flashing the current firmware.
