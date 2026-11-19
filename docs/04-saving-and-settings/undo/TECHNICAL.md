# Undo, Redo and Hierarchy Restoration — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Metadata transaction integration

Unreal `FMetaData` is not a normal automatically transacted UObject property. Index uses `FHierarchyChange : FCommandChange` and `FNativeHierarchyUndoBaselineChange` to record before/after serialized hierarchy values against `ULevel`.

## Reconciliation after Undo

`FEditorDelegates::PostUndoRedo` triggers `PrepareForPostUndoRedo()` and refreshes Outliners. Source logic avoids reconciling partially restored rows while `GIsTransacting` is true. Its native-move baseline and departure-tier snapshots preserve the original sibling sequence when callbacks arrive incrementally.

## Test coverage already authored

`Index.HierarchyState` includes automated spec cases for initial capture, move-before preserving visual source order, top-of-tier insertion, native incremental moves, restored sibling slots, and rename independence. These are **test definitions in the repository**, not tests executed in this documentation pass.

## Caution

Duplication and native Folder operations may have distinct transactions from Index's final sequence reconciliation, so one Undo isn't guaranteed to reverse every visible effect.

---

[Read Design](DESIGN.md) · [Source](https://github.com/mippi-the-dork/Index/tree/main/Source/Index) · Execution testing not yet verified.
