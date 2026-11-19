# Dropping into Empty Outliner Space — Technical

**Index v1.0.0 · Source-based reference**

## Root target resolution

`IndexInteraction::FindRootDropTarget` identifies blank space in the actual registered Outliner tree, separate from row-edge or center hierarchy targets. During hover it caches `CachedRootTarget` and displays an insertion line using the Below color.

## Mouse release

The input preprocessor handles the cached target by cancelling the original stock drag and calling `FIndexSequenceState::MoveItemsToRootAtBottom` with the selected sequence roots, associated `ULevel` and root object. On success it refreshes registered Outliners.

## Stable hierarchy semantics

The root context and Level are explicit arguments. `MoveItemsToRootAtBottom` changes the actual eligible parent/tier relationship, while preserving the moved block's own descendants and relative sequence.

## Safe classification

A release in ordinary blank screen space outside the recognized Outliner is not a root-drop operation. The gesture requires the correct widget path, payload and level compatibility. Test with multiple Outliner windows and floating layouts.

---

[Design guide](DESIGN.md) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp) · [IndexModule.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexModule.cpp).

**Verification:** This is a source review; live-editor execution remains unverified here.
