# Your First Reorder — Technical

**Index v1.0.0 · Implementation reference**

## How the gesture is recognized

`IndexInteraction::ResolveZone` converts pointer screen coordinates into a normalized fraction of a target row's cached Slate geometry. With no modifier it classifies values less than or equal to `EdgeThresholdPercent/100` as Before and greater than or equal to its complement as After. The center stays native.

`IndexInteraction::FindDropTarget` validates that both the moved payload and destination are supported sequence items, then consults `FIndexSequenceState::CanInsertItemsRelative`. Hover records a target in the input preprocessor; the release uses that validated target to call `InsertItemsRelative` and refreshes registered Outliners.

## UI signals

`FIndexLineOverlay` draws the insertion line; colors come from `UIndexSettings::AboveLineColor` and `BelowLineColor`. Index does not replace native row highlight/selection brushes.

## Data write

`FIndexSequenceState::InsertItemsRelative` changes the child sequence of a compatible level parent and saves the serialized hierarchy into `Index.Hierarchy.V1` metadata. A successful change marks the containing level package dirty and stores transaction data for Undo.

## First test's limits

A successful Outliner reorder does not show whether World Partition streaming, duplicate reconstruction or multi-level constraints have been exercised. Those cases have separate [Verification Matrix](../../VERIFICATION_MATRIX.md) entries.

Sources: [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp), [IndexLineOverlay.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexLineOverlay.cpp).

---

For expected user behavior, [read Design](DESIGN.md). Source: [Index repository](https://github.com/mippi-the-dork/Index/tree/main/Source/Index).

**Verification:** Reviewed against published source; in-editor QA is tracked separately.
