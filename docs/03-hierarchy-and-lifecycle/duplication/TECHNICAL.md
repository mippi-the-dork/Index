# Duplicating Actors and Folders — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Duplicate source snapshot

`FIndexModule` uses `OnDuplicateActorsBegin` and `OnDuplicateActorsEnd` to capture selected sequence roots before the native duplication and identify new objects afterward.

## Deferred finalization

Folder duplication can finish rebuilding its Outliner rows after the native event. The module uses a bounded `FTSTicker` retry to distinguish new duplicate rows from original selected roots before final placement.

`CollapseToTopLevelRoots` and `SortItemsForMove` normalize both blocks. If all copies share a compatible source `FIndexSequenceGroupKey`, `InsertItemsRelative` positions them after the source; otherwise `IntegrateItemsAtTop` handles the fallback.

## Native command ownership

Folder selections may route Ctrl+D to Unreal's hierarchy-aware Folder duplicate operation. Actor-only duplication follows Unreal's normal Actor path. Index only reconciles the resulting Outliner order.

## Undo boundary

The native duplication and final Index insertion can be separate transactions. This is a documented behavior limit, not a promise that a duplicate operation is always one Undo step.

---

For the workflow, [read Design](DESIGN.md). [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp).

**Validation:** Source review is not a passed Unreal Editor test.
