# Stable Identity and Reconciliation — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Stable row identifiers

`FIndexSequenceItem::FromActor` uses `AActor::GetActorGuid()`; Folders use `FFolder::GetActorFolderGuid()` where valid with root/parent context. `FIndexSequenceGroupKey` combines associated Level, root and either parent Folder or parent Actor information.

## Canonical snapshot

`FCachedLevelState::ChildrenByParent` is a map of parent identifiers to ordered child IDs, serialized under `Index.Hierarchy.V1`. `EnsureCaptured` merges the current sibling items with the stored order rather than blindly sorting every displayed label.

## Native move baseline

`FNativeMoveBaseline` maintains pre-move visual paths, parent/group identity and exact departure-tier sibling sequences. These survive short incremental native move bursts so a later callback doesn't reverse source selection order.

## Undo anchor

`FNativeHierarchyUndoBaselineChange` stores the pre-command hierarchy snapshot across tightly grouped native transactions. During Undo replay `GIsTransacting` suppresses intermediate reconciliation. `PostUndoRedo` refreshes from settled restored state.

## Source

The core is [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp), notably `GetGroupKey`, `EnsureCaptured`, `InsertItemsRelativeImpl`, `Save`, and the FCommandChange subclasses.

## Verification boundary

The repository includes an Automation Spec for selected algorithms. Live World Partition/large-level stability and all native paths require separate runtime testing.

---

[Read Design](DESIGN.md) · [Index source](https://github.com/mippi-the-dork/Index/tree/main/Source/Index) · Unreal Editor execution has not been verified in this pass.
