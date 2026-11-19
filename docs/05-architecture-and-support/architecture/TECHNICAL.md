# Editor Architecture — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Module and interfaces

`FIndexModule::StartupModule` registers an Actor Browser column through `FSceneOutlinerModule`, installs a Slate `IInputProcessor` and subscribes to Editor, Actor, Folder and map lifecycle events.

`FIndexColumn` implements `SortItems` and registers its Outliner. Its header is deliberately hidden and its row widget is `SNullWidget`, leaving native row layout untouched.

## Input and presentation

`IndexInteraction` resolves row/drop regions, native hierarchy actions, multi-selection roots, and cached validated targets. `FIndexLineOverlay` draws input-transparent full-width insertion previews. Drag tooltips use native decorated operations.

## State

`FIndexSequenceState` reconciles per-parent ordered child lists with loaded Actor/Folder rows, records stable identities and saves the `Index.Hierarchy.V1` snapshot on the Level package.

## Lifecycle

Map-change and Actor-replacement delegates invalidate the cache. New Actor, duplicate and Folder delete/move callbacks maintain consistent order. On shutdown all delegates and the input processor are unregistered.

## Engine boundary

No engine source modification or replacement `FActorBrowsingMode` is required. Public engine hooks are used; other custom Outliners aren't automatically supported.

---

[Read Design](DESIGN.md) · [Index source](https://github.com/mippi-the-dork/Index/tree/main/Source/Index) · Unreal Editor execution has not been verified in this pass.
