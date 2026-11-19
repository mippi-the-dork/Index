# Before and After Drop Zones — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Zone math

`IndexInteraction::ResolveZone` obtains the row's `FGeometry`, converts cursor position with `AbsoluteToLocal`, normalizes vertical position to `Fraction`, then checks the configured `EdgeThresholdPercent/100` clamped to **0.01–0.45**.

A fraction at/below the threshold resolves `EDropZone::Before`, one at/above its inverse resolves `After`; remaining values return `Native`. Unsupported or zero-height rows stay native. This boundary logic is independent from the modifier-driven half-row mode.

## Actual target selection

`FindDropTarget` finds an `SSceneOutlinerTreeRow` under the pointer and obtains the item through the Outliner tree. Only valid Actor/Folder `FIndexSequenceItem` payloads and insertable target relationships are accepted.

For the **After** zone on an expanded parent with a visible first child, Index substitutes that first child as the target with a Before insertion. This preserves the intended child-tier insertion at the visual gap.

## Data mutation

`FIndexSequenceState::CanInsertItemsRelative` validates a legal target, and `InsertItemsRelative` commits a sequence change and triggers registered Outliner refreshes. Invalid targets are not treated as successful reorders.

## Caveat

This is specific to Index's explicit edge insertion. It does not grant permission for unrelated Unreal attachment/folder changes or cross-level movement.

---

[User-facing Design guide](DESIGN.md) · [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp)

**Validation:** Source inspected; live-editor tests remain separate.
