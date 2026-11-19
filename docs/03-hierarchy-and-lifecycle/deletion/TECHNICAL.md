# Deleting and Removing Items — Technical

**Index v1.0.0 · Source-based reference**

## Event cleanup

`FIndexModule` handles `FActorFolders::OnFolderDeleted` and `GEngine->OnLevelActorDeleted`. The appropriate `FIndexSequenceItem` is removed from the saved hierarchy through `FIndexSequenceState::RemoveItem`, followed by Outliner refresh.

## Sibling preservation

`RemoveIdEverywhere` and related state functions remove the deleted identity from child arrays without sorting the remaining children. Parent-specific sequences maintain their relative order.

## Undo interaction

Native Undo may restore deleted Actors or Folders, with reconstructed UObject pointers and the same stable GUIDs. Index has transient historical-path and departure-tier data to restore positions when the deleted/restored object reappears.

## Limits

Removal from Index metadata is not equivalent to deleting native Actors and is not an independent trash system. The native editor performs the actual destructive operation; Index responds by updating its ordering snapshot.

---

[Design guide](DESIGN.md) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp) · [IndexModule.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexModule.cpp).

**Verification:** This is a source review; live-editor execution remains unverified here.
