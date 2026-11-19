# New Actors and Folders — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Actor placement hook

`FIndexModule` listens to `FEditorDelegates::OnNewActorsPlaced`. Unless a duplication operation is active, it calls `IndexInteraction::IntegrateNewActorsAtTop`, which wraps the new Actors as `FIndexSequenceItem` entries and calls `FIndexSequenceState::IntegrateItemsAtTop`.

## Folder creation ownership

Index intentionally does not replace `SSceneOutliner::CreateFolder()` or `FActorBrowsingMode::CreateNewFolder()`, nor does it force a second refresh during native inline rename. It reconciles Folder rows through the regular hidden sorting column after Unreal has created the hierarchy.

## Separate parent groups

`GetGroupKey` chooses the actual associated Level, root object, Folder or Actor attachment. Integration at “top” means index zero within that **parent's sequence**, not index zero of a global flat list.

## Duplication guard

The module tracks `bDuplicateInProgress` to prevent the generic new-item path from taking over during a duplication operation. The duplicate subsystem later places copies after their source when possible.

---

For the workflow, [read Design](DESIGN.md). [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp).

**Validation:** Source review is not a passed Unreal Editor test.
