# Insertion Lines and Drag Feedback — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Overlay ownership

`FIndexLineOverlay` draws an input-transparent Slate popup overlay spanning the Outliner tree viewport at the target boundary, using the target row's cached geometry and the configured `AboveLineColor` or `BelowLineColor`.

`IndexInteraction::HandleMouseMoveEvent` resolves candidate target actions. For explicit ordering it calls `Overlay.ShowLine(...)` and stores the validated target. For native/special hierarchy actions it hides the insertion line and updates the existing decorated drag tooltip.

## Validated preview

`CachedTarget`, `CachedSpecialTarget`, `CachedRootTarget`, and native preview state are distinct paths. This prevents a stale edge insertion from being applied after pointer movement resolves to a different kind of action.

On release, only the currently cached valid operation executes. A custom insertion consumes the native release, cancels the stock drag, applies the explicit sequence change and refreshes the Outliner.

## Performance and cleanup

Input processing reacts during drag gestures; the popup doesn't act as a new persistent Outliner column. Shutdown unregisters the input preprocessor and the overlay is hidden when a gesture ends. Test on dense Outliners to confirm no visual artifact remains after cancellation.

Source: [IndexLineOverlay.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexLineOverlay.cpp).

---

[User-facing Design guide](DESIGN.md) · [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp)

**Validation:** Source inspected; live-editor tests remain separate.
