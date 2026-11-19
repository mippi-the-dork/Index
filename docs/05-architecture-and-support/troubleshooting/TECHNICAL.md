# Troubleshooting and Support — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Diagnostic order

1. Check that the sole Editor module loaded and the Actor Browser Outliner registered its hidden column.
2. Inspect `UIndexSettings::bEnableCustomOrdering`, Edge Threshold and modifier.
3. Determine whether `ResolveZone` classified a native center or explicit Before/After target.
4. Verify Actor/Folder payload and target `FIndexSequenceItem` validity and `CanInsertItemsRelative`.
5. Confirm the action mutated the correct `FIndexSequenceGroupKey` and `ULevel`.
6. Compare in-memory sequence with `Index.Hierarchy.V1` on the Level package after saving.
7. For Undo, review command-change replay and native callback reconciliation timing.

## Reproduction matrix

At minimum test one root Actor, one nested Folder, one attached Actor, one selected block with descendants, a duplicated Folder, and a WP loaded/unloaded pair. Confirm save, restart, Undo/Redo, and multi-Outliner refresh independently.

## Automation available in source

The `Index.HierarchyState` Automation Spec registers an Editor-context engine-filter suite for captured sibling order, reverse-input block moves, top insertion, incrementally moved Actors, restored historical positions and rename independence. Suggested console invocation: `Automation RunTests Index`.

**No automation was executed in this documentation pass.** Passing tests must be recorded only after running the suite with the intended UE version.

## Crash/ownership checks

On unload, the module unregisters Slate input processing, duplicate finalization ticker and editor/Actor/Folder delegates. A repro involving disable/re-enable should record module lifecycle and any stale input widgets/overlay state.

---

[Read Design](DESIGN.md) · [Index source](https://github.com/mippi-the-dork/Index/tree/main/Source/Index) · Unreal Editor execution has not been verified in this pass.
