# Layer Activation Rules Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Add generic transient/persistent layer activation events and rule-based companion audio actions with optional per-rule looping.

**Architecture:** Firmware continues to send queued Raw HID layer events, but class 1 gains an activation-kind byte and the cache treats same-layer kind changes as distinct. The Swift protocol target decodes the kind, the core target persists audio rules, and the app target evaluates rules and manages one-shot or looped playback. Existing per-layer sounds migrate to compatibility `entry` one-shot rules so current behavior is preserved.

**Tech Stack:** QMK C keymap, Raw HID, Swift 5.9 Package Manager, Combine, SwiftUI, AudioToolbox, AVFoundation, XCTest.

---

### Task 1: Protocol Layer Activation Kind

**Files:**
- Modify: `companion/Sources/NPE633Protocol/CompanionProtocol.swift`
- Modify: `companion/Tests/NPE633ProtocolTests/CompanionProtocolTests.swift`
- Modify: `companion/Sources/NPE633CompanionChecks/main.swift`

- [ ] **Step 1: Write failing protocol tests**

Add tests that expect one-byte legacy layer payloads to decode as `.current` and two-byte payloads to decode the activation kind:

```swift
func testLayerPayloadDefaultsActivationKindForLegacyPackets() throws {
    var state = CompanionState()
    state.apply(try CompanionPacket(bytes: eventBytes(eventClass: .layer, payload: [2])))

    XCTAssertEqual(state.activeLayer, 2)
    XCTAssertEqual(state.layerActivationKind, .current)
}

func testLayerPayloadDecodesActivationKind() throws {
    var state = CompanionState()
    state.apply(try CompanionPacket(bytes: eventBytes(eventClass: .layer, payload: [3, LayerActivationKind.transient.rawValue])))
    XCTAssertEqual(state.activeLayer, 3)
    XCTAssertEqual(state.layerActivationKind, .transient)

    state.apply(try CompanionPacket(bytes: eventBytes(eventClass: .layer, payload: [3, LayerActivationKind.persistent.rawValue])))
    XCTAssertEqual(state.activeLayer, 3)
    XCTAssertEqual(state.layerActivationKind, .persistent)
}
```

- [ ] **Step 2: Run protocol tests to verify failure**

Run: `swift test --package-path companion --filter CompanionProtocolTests/testLayerPayload`

Expected: FAIL because `LayerActivationKind` and `layerActivationKind` do not exist.

- [ ] **Step 3: Implement minimal protocol decoding**

Add `LayerActivationKind` as a public `UInt8` enum, add `layerActivationKind` to `CompanionState`, and decode layer payload offset 1 when present:

```swift
public enum LayerActivationKind: UInt8, Codable, Equatable, Sendable {
    case current = 0
    case transient = 1
    case persistent = 2
}
```

In the `.layer` reducer:

```swift
activeLayer = layer
layerActivationKind = packet.payload.count >= 2
    ? LayerActivationKind(rawValue: packet.payload[1]) ?? .current
    : .current
```

- [ ] **Step 4: Run protocol tests to verify pass**

Run: `swift test --package-path companion --filter CompanionProtocolTests`

Expected: PASS.

### Task 2: Core Audio Rule Model And Migration

**Files:**
- Modify: `companion/Sources/NPE633CompanionCore/LayerConfiguration.swift`
- Modify: `companion/Sources/NPE633CompanionCore/LayerConfigurationStore.swift`
- Modify: `companion/Tests/NPE633CompanionCoreTests/LayerConfigurationStoreTests.swift`
- Modify: `companion/Sources/NPE633CompanionChecks/main.swift`

- [ ] **Step 1: Write failing core tests**

Add tests for default compatibility rules, legacy disabled sounds, trigger-specific rules, and loop default:

```swift
func testDefaultAudioRulePreservesExistingLayerSound() {
    let store = LayerConfigurationStore(defaults: UserDefaults(suiteName: UUID().uuidString)!)
    let rules = store.audioRules(for: 5, trigger: .entry)

    XCTAssertEqual(rules.count, 1)
    XCTAssertEqual(rules.first?.sound, .builtIn(name: "Glass"))
    XCTAssertEqual(rules.first?.playback, .oneShot)
    XCTAssertTrue(rules.first?.isEnabled == true)
}

func testDisabledLegacySoundHasNoEnabledAudioRule() {
    let defaults = UserDefaults(suiteName: UUID().uuidString)!
    defaults.set(SoundPreferences.disabledSentinel, forKey: SoundPreferences.legacyKeyPrefix + "3")
    let store = LayerConfigurationStore(defaults: defaults)

    XCTAssertTrue(store.audioRules(for: 3, trigger: .entry).isEmpty)
}

func testPersistsTriggerSpecificLoopAudioRule() {
    let defaults = UserDefaults(suiteName: UUID().uuidString)!
    let store = LayerConfigurationStore(defaults: defaults)
    let rule = LayerAudioRule(trigger: .persistentEntry, sound: .builtIn(name: "Ping"), playback: .loopWhileActive)

    store.setAudioRules([rule], forLayer: 2)

    let reloaded = LayerConfigurationStore(defaults: defaults)
    XCTAssertEqual(reloaded.audioRules(for: 2, trigger: .persistentEntry), [rule])
    XCTAssertTrue(reloaded.audioRules(for: 2, trigger: .transientEntry).isEmpty)
}
```

- [ ] **Step 2: Run core tests to verify failure**

Run: `swift test --package-path companion --filter LayerConfigurationStoreTests/testDefaultAudioRulePreservesExistingLayerSound`

Expected: FAIL because `LayerAudioRule`, triggers, playback, and store APIs do not exist.

- [ ] **Step 3: Implement rule types and persistence**

Add these public types:

```swift
public enum LayerRuleTrigger: String, Codable, CaseIterable, Equatable, Sendable {
    case entry
    case transientEntry
    case persistentEntry
}

public enum LayerAudioPlayback: String, Codable, CaseIterable, Equatable, Sendable {
    case oneShot
    case loopWhileActive
}

public struct LayerAudioRule: Codable, Equatable, Sendable, Identifiable {
    public var id: UUID
    public var trigger: LayerRuleTrigger
    public var sound: LayerSoundSelection
    public var playback: LayerAudioPlayback
    public var isEnabled: Bool
}
```

Add `[LayerAudioRule]` to `LayerConfiguration` with decoding default derived from `sound`, keep existing `sound` and `soundTransition` for compatibility, and add store APIs:

```swift
public func audioRules(for layer: UInt8, trigger: LayerRuleTrigger) -> [LayerAudioRule]
public func setAudioRules(_ rules: [LayerAudioRule], forLayer layer: UInt8)
public func setAudioRule(_ rule: LayerAudioRule, forLayer layer: UInt8)
public func removeAudioRule(id: UUID, forLayer layer: UInt8)
```

- [ ] **Step 4: Run core tests to verify pass**

Run: `swift test --package-path companion --filter LayerConfigurationStoreTests`

Expected: PASS.

### Task 3: Rule Evaluation And Playback Controller

**Files:**
- Modify: `companion/Sources/NPE633CompanionApp/LayerSoundController.swift`
- Modify: `companion/Sources/NPE633CompanionApp/CompanionRuntimeController.swift`

- [ ] **Step 1: Add controller behavior for layer events**

Replace `observeLayer(_:)` with an event-shaped API:

```swift
public func observeLayer(_ layer: UInt8, activationKind: LayerActivationKind)
```

Keep the existing `observeLayer(_:)` overload as a compatibility shim that calls `.current`.

- [ ] **Step 2: Implement trigger evaluation**

When the first event arrives, seed `lastLayer` and `lastActivationKind` without playing. On later events:

- If layer changed, evaluate `.entry`.
- If kind is `.transient`, evaluate `.transientEntry`.
- If kind is `.persistent`, evaluate `.persistentEntry`, including same-layer promotion.
- Suppress duplicate `.entry` for same-layer promotion.

- [ ] **Step 3: Implement loop slots**

Track active loop players by rule ID. For `oneShot`, reuse the current one-shot path. For `loopWhileActive`, use `AVAudioPlayer`, set `numberOfLoops = -1`, and stop loops from the previous layer when the layer changes or the global sound toggle disables sound.

- [ ] **Step 4: Update runtime observation**

Change the Combine pipeline from `.map(\.activeLayer).removeDuplicates()` to observing a layer event identity that includes `activeLayer`, `layerActivationKind`, and `lastSequence`, then call:

```swift
soundController?.observeLayer(state.activeLayer, activationKind: state.layerActivationKind)
```

- [ ] **Step 5: Build app target**

Run: `swift build --package-path companion`

Expected: PASS.

### Task 4: Rule Editing UI

**Files:**
- Modify: `companion/Sources/NPE633CompanionApp/CompanionView.swift`

- [ ] **Step 1: Replace transition picker with audio-rule summary**

In the layer configuration table, replace the `Transition` column with `Audio Rules`. The row shows enabled rule count and an edit button.

- [ ] **Step 2: Add per-layer rule editor sheet**

Add a sheet that can add, enable, disable, delete, preview, and edit rules with trigger, sound, and playback pickers.

- [ ] **Step 3: Build app target**

Run: `swift build --package-path companion`

Expected: PASS.

### Task 5: Firmware Layer Activation Payload

**Files:**
- Modify: `keymaps/default/keymap.c`
- Modify: `docs/companion-raw-hid.md`

- [ ] **Step 1: Extend companion layer payload**

Add layer activation constants:

```c
enum companion_layer_activation_kind {
    COMPANION_LAYER_ACTIVATION_CURRENT,
    COMPANION_LAYER_ACTIVATION_TRANSIENT,
    COMPANION_LAYER_ACTIVATION_PERSISTENT,
};
```

Queue and send both layer state and activation kind. Class 1 payload becomes `{active_layer, activation_kind}`.

- [ ] **Step 2: Preserve same-layer promotion events**

Include activation kind in the layer event cache comparison so `layer=NAV transient` followed by `layer=NAV persistent` is sent.

- [ ] **Step 3: Classify known layer keys**

In `process_record_user()`, record pending activation hints for `MO`, `TT`, `TG`, and `TO` keycodes used by this keymap. Ensure `TT` can emit transient first and persistent when QMK promotes the same layer.

- [ ] **Step 4: Update Raw HID docs**

Document class 1 offset 1 and activation-kind values.

- [ ] **Step 5: Compile firmware**

Run: `qmk compile -kb handwired/npe633_cosmotyl -km default`

Expected: PASS.

### Task 6: Final Verification

**Files:**
- Verify all changed files.

- [ ] **Step 1: Run Swift tests**

Run: `swift test --package-path companion`

Expected: PASS.

- [ ] **Step 2: Run companion checks**

Run: `swift run --package-path companion NPE633CompanionChecks`

Expected: PASS.

- [ ] **Step 3: Run firmware compile**

Run: `qmk compile -kb handwired/npe633_cosmotyl -km default`

Expected: PASS.

- [ ] **Step 4: Review diffs**

Run: `git status --short` and inspect changes. Confirm pre-existing user changes in `docs/status-indicator.md` and unrelated `keymap.c` sections are not reverted.
