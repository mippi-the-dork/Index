# Parenting and Moving Into Folders — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Gesture classification

`IndexInteraction::FindSpecialDropTarget` recognizes native parent/Folder operations separately from edge insertion. A valid special target is cached during hover while the insertion line is hidden. Mouse release dispatches the proper native hierarchy operation using an `ApplyParentToTarget`-style action, cancelling the overlapping original drag as required.

## Hierarchy ownership

Unreal owns `AActor::GetAttachParentActor()`, Actor Folder membership and `FActorFolders`. Index reads that state through `FIndexSequenceState::GetGroupKey` to know which local sibling sequence is being changed.

## Reconciliation after native moves

An Actor moved into a Folder departs one sibling tier and enters another. Index records pre-move information and reconciles the appropriate parent-specific sequences when native callbacks arrive, retaining the order of moved blocks.

## Limits and verification

Native validation still governs legal attachment/Folder relationships; Index's sequencing layer doesn't create a new hierarchy model. Test center placement separately from top-edge and lower-edge placement, particularly for selected blocks and nested Folders.

---

For the workflow, [read Design](DESIGN.md). [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp).

**Validation:** Source review is not a passed Unreal Editor test.
