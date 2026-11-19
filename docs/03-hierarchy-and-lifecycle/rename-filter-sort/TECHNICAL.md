# Renaming, Filtering and Native Sorting — Technical

**Index v1.0.0 · Source-based reference**

## Stable identity is not the display label

`FIndexSequenceItem::FromActor` and `FromFolder` derive stable identities from Actor GUIDs and persistent Folder GUIDs when available. The canonical child lists are keyed by parent/group identity, not by display-string alphabetization.

## Sorting and filtering

`FIndexColumn::SortItems` asks `FIndexSequenceState` to reconcile the entire current sibling list before applying manual comparisons. It is invoked by Scene Outliner's sort lifecycle, not a custom tree view.

Unrecognized non-Index row types can retain priority/text fallback ordering. Search filters remove rows from presentation; the snapshot is not reconstructed solely from visible search results.

## Other column sorting

Native Outliner sorting can temporarily control the visible order. A new Index operation refreshes registered Outliners using the Index sequence. A sorting choice should not be described as deleting metadata.

## Rebuild and blueprint replacement

The module listens to `OnObjectsReplaced` and invalidates cached sequence state so replacement Actors are matched against their saved GUID-based identity instead of a stale UObject pointer.

---

[Design guide](DESIGN.md) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp) · [IndexModule.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexModule.cpp).

**Verification:** This is a source review; live-editor execution remains unverified here.
