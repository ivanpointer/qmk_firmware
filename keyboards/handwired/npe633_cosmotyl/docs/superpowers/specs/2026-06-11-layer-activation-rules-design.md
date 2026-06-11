# Layer Activation Rules Design

## Context

The NPE633 companion app currently plays a configured layer sound whenever the keyboard reports a changed active layer. The firmware's Raw HID class 1 layer payload only reports the active layer ID, so the app cannot tell whether that layer was reached through a temporary hold, a tap-toggle promotion, a normal toggle, or a full layer move.

The new behavior should keep the existing default audio behavior while exposing more precise layer activation semantics. The protocol should describe keyboard state, not audio. Audio playback should become one consumer of a more general rule mechanism.

## Goals

- Preserve existing layer-change sound behavior by default.
- Report whether a layer activation is transient or persistent when the firmware can determine that.
- Emit a second event when a temporary activation is promoted to persistent, such as a `TT(layer)` tap-toggle reaching its toggle threshold.
- Let the companion configure separate actions for transient entry and persistent entry of the same layer.
- Let each audio action choose one-shot or looped playback independently.
- Keep loop playback off by default.
- Avoid blocking QMK key processing with Raw HID work; preserve the existing queued companion layer event path.

## Non-Goals

- Do not add host-controlled layer changes.
- Do not persist rule configuration to keyboard EEPROM.
- Do not change status LED layer behavior.
- Do not require existing companion preferences to be reset.

## Protocol

Class 1 remains the layer state class. Its payload expands from one byte to two bytes:

| Offset | Name | Description |
| --- | --- | --- |
| 0 | Active layer ID | Highest active layer after applying layer and default-layer state |
| 1 | Activation kind | `0` current/unspecified, `1` transient, `2` persistent |

Older one-byte layer payloads remain valid in the app and decode as `current/unspecified`.

Activation kind meanings:

- `current/unspecified`: A status snapshot or compatibility event that reports current state without claiming why the layer is active.
- `transient`: The active layer is entered through a hold-style activation that is expected to revert when the key is released, such as `MO`, held `TT`, held `LT`, or `LM`.
- `persistent`: The active layer is entered or promoted through state that is not expected to revert on key release, such as `TG`, `TO`, default-layer changes, or the final `TT` tap that toggles the layer on.

For `TT(layer)`, the firmware should send a transient event as soon as the layer is entered for hold behavior, then send a persistent event if the tap-toggle threshold promotes that same active layer to toggled-on state.

## Firmware Design

Firmware keeps `layer_state_set_user()` lightweight by queueing companion layer events for `housekeeping_task_user()`, matching the existing non-blocking Raw HID design.

The queued layer event should carry both the target layer state and the best-known activation kind. Existing generic layer-state changes that cannot be tied to a specific key action may queue `current/unspecified`.

The firmware should classify known keycodes in `process_record_user()` before QMK's normal layer action runs:

- `MO`, held `TT`, held `LT`, and `LM`: transient.
- `TG`, `TO`, and persistent `TT` toggle: persistent.
- `DF` and persistent default-layer movement: persistent if reflected in the active layer event.

Where QMK only exposes the resulting layer state through `layer_state_set_user()`, the keymap can track a small pending activation-kind hint by layer. `layer_state_set_user()` then attaches that hint to the queued companion event. If no hint matches the active layer change, it falls back to `current/unspecified`.

The companion layer event cache must include activation kind. A repeated active layer with a different activation kind is a real event, so `layer=NAV transient` followed by `layer=NAV persistent` must both be sent.

## Companion State

`CompanionState` gains a `layerActivationKind` property. The state reducer decodes payload offset 1 when present and otherwise uses `current/unspecified`.

Runtime observation must stop deduplicating only on `activeLayer`. It should observe a lightweight layer event identity, such as `(activeLayer, layerActivationKind, sequence)`, so a promotion event for the same layer reaches consumers.

Consumers that only care about display state can continue reading `activeLayer`.

## Rule Model

Layer configuration moves from a single layer sound plus transition toggle toward a list of rules. The first rule type is audio.

Each rule has:

- Layer ID.
- Trigger.
- Sound selection.
- Playback mode.
- Enabled flag.

Initial audio triggers:

- `entry`: fires when the active layer changes to the configured layer, regardless of activation kind. This is the compatibility trigger.
- `transientEntry`: fires when the active layer changes to the configured layer with activation kind `transient`.
- `persistentEntry`: fires when the layer is entered or promoted with activation kind `persistent`, including same-layer transient-to-persistent promotion.

Initial playback modes:

- `oneShot`: play once.
- `loopWhileActive`: loop until the layer leaves or the matching rule is superseded.

Loop playback defaults to off because migrated and default rules use `oneShot`.

## Preference Migration

Existing per-layer sound settings migrate to one enabled compatibility rule:

- Trigger: `entry`.
- Sound: the existing `LayerSoundSelection`.
- Playback mode: `oneShot`.

Existing disabled layer sounds migrate to no enabled audio rule for that layer.

Existing `soundTransition` settings continue to suppress compatibility entry rules for the affected layer during migration. New transient and persistent rules should not need the old transition field; the UI can hide or deprecate it after migration.

This preserves current behavior: a configured layer sound still plays once when the active layer changes, regardless of whether the underlying activation was transient or persistent.

## Audio Playback

One-shot playback can continue to use cached system sounds where supported.

Loop playback must use `AVAudioPlayer`, because `AudioServicesPlaySystemSound` is one-shot. A loop rule should maintain a stable active loop slot keyed by layer and rule ID. When the rule starts, the controller creates or reuses an `AVAudioPlayer`, sets `numberOfLoops = -1`, resets `currentTime`, and starts playback.

When the active layer leaves, the controller stops all loop slots for the previous active layer. On transient-to-persistent promotion for the same layer, transient loops should stop before persistent rules start unless the user has configured a separate persistent loop.

If a new loop starts for the same layer and trigger, it replaces the previous loop. Global sound mute stops active loops and prevents new one-shots.

## UI

The layer configuration table should expose audio rules per layer without making the grid overly wide.

Recommended first UI:

- Keep the layer row compact with a summary such as `2 audio rules`.
- Add an edit button that opens a per-layer rule editor sheet.
- The sheet lists rules with trigger, sound, playback mode, enabled toggle, preview, duplicate, and delete controls.
- Add rule defaults to make common setup fast:
  - Add transient one-shot.
  - Add persistent loop.
  - Add compatibility entry one-shot.

The current layer preview can still preview the selected layer's first enabled compatibility or trigger-specific sound, but rule-level preview is the source of truth.

## Testing

Firmware tests are mostly compile-level for this keymap, with targeted manual validation using the mock or real companion:

- `MO`/held `TT` emits transient entry promptly.
- `TT` promotion emits persistent for the same layer.
- `TG`/`TO` emits persistent.
- Same-layer transient-to-persistent events are not collapsed by the firmware cache.
- QMK compile still passes.

Companion tests should cover:

- One-byte layer payload decodes as `current/unspecified`.
- Two-byte layer payload decodes activation kind.
- State observation delivers same-layer activation-kind changes.
- Existing preferences migrate to `entry` + `oneShot`.
- Disabled layer sounds do not produce enabled rules.
- `entry` compatibility rule preserves old behavior.
- `persistentEntry` fires on same-layer promotion.
- Loop rules start and stop when expected.

## Open Implementation Notes

The companion app source is symlinked from this keyboard directory to `npe633-cosmotyl-companion`. Implementation may require working in both repositories: firmware/protocol documentation in QMK, and Swift protocol/config/playback/UI changes in the companion package.
