# Actors and Folders in One Order — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Parent-specific data model

`FIndexSequenceState::GetGroupKey` identifies the sibling tier using the associated `ULevel`, root object, Folder path/GUID or attached parent Actor. Root siblings, Folder children and attached Actor children are distinct groups.

Saved sequences are maps of **parent identifier → ordered child identifiers**, not numeric sequence properties on individual Actor/Folder objects. This representation naturally supports interleaving Actor and Folder items where native Outliner relationships permit it.

## Stable identity

Actor entries use `AActor::GetActorGuid()`; folders use `FFolder::GetActorFolderGuid()` where valid, with parent paths/root context as necessary. Renaming a label does not cause the sequence to be regenerated alphabetically.

## Sort path

`FIndexColumn::SortItems` is called per sibling list. It records the current list with `EnsureCaptured` before comparing rows using `FIndexSequenceState::Less`. Non-Index row types retain native priority/string fallbacks.

## Critical limitation

A row's visual order is not equivalent to its transform, attachment or level ownership. Index only changes the canonical sibling order, unless the gesture explicitly performs a native hierarchy move.

---

[User-facing Design guide](DESIGN.md) · [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp)

**Validation:** Source inspected; live-editor tests remain separate.
