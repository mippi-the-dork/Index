# Saving and Restoring Manual Order — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Serialization ownership

`FIndexSequenceState` stores the hierarchy snapshot on the `ULevel` package using `FMetaData` key `Index.Hierarchy.V1`. The text payload starts with `V1`, then lists parent keys and comma-separated child IDs, preserving distinct sibling tiers.

## Write semantics

`Save(ULevel*,FCachedLevelState&)` compares the serialized value with the last persisted value, records an `FHierarchyChange` in the active Undo buffer when applicable, calls `Meta.SetValue(Level, ...)`, and marks the Level package dirty.

## Source-control implications

The metadata isn't written into each external Actor package, nor into a separate user asset. The Level package containing `ULevel` owns the snapshot. A project can still dirty additional native assets for unrelated operations.

## Reconciliation on loading

`EnsureCaptured` and its cache reconcile current sibling rows with saved parent-child sequences using stable Actor/Folder identities. Map changes invalidate cached state. Filtering does not serialize a truncated sequence.

**Validation limit:** The described path is source-reviewed; live Perforce/OFPA save, checkout and reload testing is not claimed complete.

---

[Read Design](DESIGN.md) · [Source](https://github.com/mippi-the-dork/Index/tree/main/Source/Index) · Execution testing not yet verified.
