# What Index Does — Technical

**Index v1.0.0 · Implementation reference**

## Integration boundary

`Index.uplugin` registers the `Index` module as `Editor` at `PostEngineInit`, with a Win64 allowlist. No runtime module or Content Browser asset is required.

`FIndexModule::StartupModule` registers `FIndexColumn` through `FSceneOutlinerModule::OnCreateActorBrowserColumns()`. The column is visually hidden—its header has hidden visibility and `ConstructRowWidget()` returns `SNullWidget`. It supplies `SortItems()` for the manual sibling order instead of painting new Actor labels.

## Separation of responsibilities

`IndexInteraction` observes native Slate drag/drop and manages valid before/after targets, special hierarchy actions, and insertion-line feedback. `FIndexSequenceState` resolves identities, maintains per-parent child sequences, and records serialized ordering in level package metadata.

## Non-goals

The existing Editor module does not implement world-transform editing, custom Actor classes, a packaged runtime hierarchy, or a replacement `FActorBrowsingMode`. Unreal remains responsible for Actors, attachment and Folder membership; Index owns manual sibling order.

Sources: [IndexModule.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexModule.cpp), [IndexColumn.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexColumn.cpp), and [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp).

---

For expected user behavior, [read Design](DESIGN.md). Source: [Index repository](https://github.com/mippi-the-dork/Index/tree/main/Source/Index).

**Verification:** Reviewed against published source; in-editor QA is tracked separately.
