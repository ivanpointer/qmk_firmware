# Companion Raw HID Protocol

The default keymap exposes a vendor-defined Raw HID interface for a macOS companion app. The keyboard sends typed state events when runtime state changes, and the host can request runtime status or flashed/static configuration after launch or reconnect.

Raw HID uses the QMK defaults:

| Field | Value |
| --- | --- |
| Usage page | `0xFF60` |
| Usage ID | `0x61` |
| Report size | 32 bytes |

## Packet Header

Every packet uses this 8-byte header. Unused bytes are zero.

| Offset | Name | Description |
| --- | --- | --- |
| 0 | Magic 0 | ASCII `N` |
| 1 | Magic 1 | ASCII `P` |
| 2 | Version | `1` |
| 3 | Message type | `0x01` event, `0x80` query status, `0x81` query class, `0x82` query config, `0x83` set config |
| 4 | Sequence | Keyboard-generated event sequence |
| 5 | Class | Event class or requested class |
| 6 | Flags | Event/query response flags: bit 0 local side is left, bit 1 sender is master. For `set config`, flags are class-specific write flags. |
| 7 | Payload length | Number of payload bytes at offset 8 |

Host query and config-write packets must use the same magic and version. For `query class`, put the requested class at offset 5.

## Sync Tiers

The protocol has four traffic tiers:

| Tier | Message type | Purpose |
| --- | --- | --- |
| Event updates | `COMPANION_MSG_EVENT` / `0x01` | Keyboard-pushed one-off updates for runtime changes such as active layer, lock state, trackball mode, current levels, USB/split/status, and lift state |
| Status sync | `COMPANION_MSG_QUERY_STATUS` / `0x80` | Host-requested refresh of all runtime status classes, used after connect/reconnect or if events may have been missed |
| Config sync | `COMPANION_MSG_QUERY_CONFIG` / `0x82` | Host-requested refresh of flashed/static configuration classes, used after first connect to a firmware/configuration the app has not cached |
| Config write | `COMPANION_MSG_SET_CONFIG` / `0x83` | Host-requested persistent configuration update. The keyboard validates and stores supported fields in EEPROM, then emits a targeted config refresh. |

`COMPANION_MSG_QUERY_CLASS` (`0x81`) remains available for targeted refreshes and diagnostics.

## Event Classes

| Class | Name | Payload |
| --- | --- | --- |
| 0 | Identity | Version, class count, status light count, default scroll level, default CPI level, layer count |
| 1 | Layer | Active layer ID |
| 2 | Locks | Bit 0 Caps Lock, bit 1 Num Lock, bit 2 Scroll Lock |
| 3 | Trackball | Left mode, right mode, held/latched flags |
| 4 | Levels | Current scroll level, current CPI level, flags |
| 5 | USB | Bit 0 host configured, bit 1 suspended, bit 2 startup active |
| 6 | Split | Error code, split flags, pointing device status |
| 7 | Lift | Lifted flags, valid flags, left raw motion byte, right raw motion byte |
| 8 | Status | Bit 0 status lights enabled, brightness |
| 9 | Layer color | One packet per layer: layer ID, primary HSV, secondary HSV, flags, brightness percent |
| 10 | Level table | One packet per visible level: level ID 1..7, HSV, scroll divisor, CPI value little-endian |
| 11 | Status color | One packet per semantic color: color ID, HSV, aux byte |
| 12 | Layer alias | One packet per layer: layer ID, alias length, ASCII alias |

Trackball mode IDs match `enum trackball_mode` in `keymap.c`. Layer IDs match `enum layers`.

Runtime status classes are 0..8. `query status` sends those classes and normal event updates are generated only for changes in classes 1..8. Flashed/static configuration classes are 9..12. `query config` sends those classes, and they are not emitted as normal change events.

Lock payloads report the same host LED state that drives the keyboard status lights. They are read-only companion events; the keyboard does not require the companion app to learn Caps Lock, Num Lock, or Scroll Lock changes.

## Color Payloads

HSV bytes use QMK's HSV constants directly. Runtime LED brightness is reported separately by class 8 (`Status`) and should be applied by the companion app when it wants to mirror the keyboard. Layer color payloads also include the per-layer brightness percentage used by the keyboard status LEDs.

### Identity payload, class 0

| Offset | Description |
| --- | --- |
| 0 | Protocol version |
| 1 | Event class count |
| 2 | Status light count |
| 3 | Default scroll level |
| 4 | Default CPI level |
| 5 | Layer count |

### Layer payload, class 1

Class 1 is the current active layer state. Layer names are reported separately by class 12 so normal layer-change events stay small.

| Offset | Description |
| --- | --- |
| 0 | Active layer ID |

### Layer color payload, class 9

| Offset | Description |
| --- | --- |
| 0 | Layer ID |
| 1..3 | Primary HSV |
| 4..6 | Secondary HSV |
| 7 | Flags: bit 0 means alternate between primary and secondary |
| 8 | Brightness percent |

### Layer alias payload, class 12

Class 12 is config/table data sent during config sync or explicit class queries. It gives the companion stable display aliases for each firmware layer.

| Offset | Description |
| --- | --- |
| 0 | Layer ID |
| 1 | Alias length in bytes |
| 2.. | ASCII alias bytes, not null-terminated |

### Levels payload, class 4

Class 4 is the current selection state. It is intentionally small so normal level-change events do not resend the full lookup table.

| Offset | Description |
| --- | --- |
| 0 | Current scroll level |
| 1 | Current CPI level |
| 2 | Flags: bit 0 precision mode active |

### Level table payload, class 10

Class 10 is config/table data sent during config sync or explicit class queries. The same HSV palette is used for scroll and CPI overlays. Scroll divisors and CPI values are backed by the keymap user EEPROM block; on EEPROM reset, invalid EEPROM data, or first boot with no valid table, the firmware initializes the stored table from the flashed defaults.

| Offset | Description |
| --- | --- |
| 0 | Visible level ID, 1..7 |
| 1..3 | HSV |
| 4 | Scroll divisor for this level |
| 5..6 | CPI value for this level, little-endian |

The default CPI table is:

| Visible level | Default CPI |
| --- | --- |
| 1 | 500 |
| 2 | 750 |
| 3 | 1000 |
| 4 | 2000 |
| 5 | 3000 |
| 6 | 5000 |
| 7 | 8000 |

The default scroll divisor table is:

| Visible level | Default scroll divisor |
| --- | --- |
| 1 | 192 |
| 2 | 128 |
| 3 | 96 |
| 4 | 64 |
| 5 | 40 |
| 6 | 24 |
| 7 | 12 |

### Config write payloads

`COMPANION_MSG_SET_CONFIG` uses the same header as query packets. Put the config class at offset 5 and the payload length at offset 7.

For class 10 level-table writes, send the class 10 payload shape above. The firmware can persist offsets 0, 4, and 5..6: visible level ID, scroll divisor, and CPI value. HSV bytes are ignored on writes because those colors remain flashed firmware config.

Class 10 `set config` flags:

| Flag bit | Meaning |
| --- | --- |
| 0 | Reset stored scroll and CPI tables to flashed defaults |
| 1 | Reset only stored scroll divisors to flashed defaults |
| 2 | Reset only stored CPI values to flashed defaults |
| 3 | Update the scroll divisor from payload offset 4 |
| 4 | Update the CPI value from payload offsets 5..6 |

If neither update bit 3 nor bit 4 is set and the payload is present, the firmware treats the packet as a full-row write and updates both scroll divisor and CPI. This keeps simple class 10 row writes possible while still allowing the companion to adjust one value without touching the other.

CPI writes are clamped to the PMW3389-supported `50..16000` range and rounded to the nearest `50` CPI step before storage. Scroll divisor writes are clamped to `1..255`, where lower values scroll faster. Updating the currently selected CPI level applies the new sensor CPI immediately unless precision mode is active. Updating the currently selected scroll level clears scroll accumulators immediately.

Cursor CPI and scroll speed are intentionally independent at runtime. Scroll-like modes use a fixed firmware CPI while active, then restore the selected cursor CPI when the side leaves scroll mode. This makes the scroll divisor table the source of scroll speed instead of letting cursor DPI changes implicitly alter scroll feel.

### Status color payload, class 11

| Offset | Description |
| --- | --- |
| 0 | Color ID |
| 1..3 | HSV |
| 4 | Aux byte, currently the breathe minimum for animated colors or `0` |

Status color IDs:

| ID | Meaning |
| --- | --- |
| 0 | Off |
| 1 | Default/white |
| 2 | Startup |
| 3 | Sleep |
| 4 | No host |
| 5 | Boot signal |
| 6 | Error |
| 7 | Caps Lock |
| 8 | Num Lock |
| 9 | Scroll Lock |
| 10 | Precision mode |
| `0x20 + trackball mode ID` | Trackball mode color |

## Lift State

Lift state comes from the PMW33xx motion-burst lift bit, `pmw33xx_report_t.motion.b.is_lifted`. The keymap observes that report through a weak PMW33xx user hook before QMK discards lifted motion. On split builds, the master polls the remote half through a keymap RPC transaction so host events include both left and right lifted state.

The raw motion bytes in the lift payload are diagnostic context only. Normal motion-byte changes do not trigger lift events; the keyboard sends class 7 only when lifted or valid bits change.
