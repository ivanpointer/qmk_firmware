# Status Indicator

The YD-RP2040 onboard RGB LED on GP23 shows keyboard status in this priority order:

1. Startup flash
2. Error blink code
3. Caps Lock breathing
4. Current layer color

Normal layer colors avoid red, orange, amber, pink, and magenta. Red is reserved for error blink codes. The layer palette uses saturated colors and simple alternating patterns because the LED is transported through a fiber pipe to a small chassis hole. Caps Lock is the only smooth breathing status.

## Layer Indicators

| Layer | Indicator |
| --- | --- |
| `_BASE` | Solid white |
| `_SH2` | Alternating white / blue |
| `_MOU` | Solid cyan |
| `_NAV` | Solid green |
| `_NUM` | Solid yellow |
| `_FN1` | Solid purple |
| `_FN2` | Alternating purple / cyan |
| `_SYS` | Alternating purple / yellow |

## Error Blink Codes

Error codes repeat for 30 seconds when the error first appears. If the same error remains active, the LED falls back to the normal layer or Caps Lock status so one-half testing is still usable.

| Pattern | Meaning |
| --- | --- |
| 1 red blink | Split transport is disconnected |
| 2 red blinks | Pointing device status is not success |

If multiple errors are active, the lower-numbered code has priority. A split transport error takes priority over a pointing device error.

## Console

`qmk console` requires firmware compiled with `CONSOLE_ENABLE = yes`. This keyboard enables console in `rules.mk` so the device can be found by `qmk console` after flashing the current firmware.
