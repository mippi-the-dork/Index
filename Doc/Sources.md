# Index Architecture and Sources

This document describes the implementation architecture used by Index and the Unreal Engine source areas that informed it. It is intended for maintainers and contributors.

## Core design

Index extends the standard Unreal Engine World Outliner rather than replacing it. Unreal continues to own actor attachment, folder membership, selection, rename, filtering, and the normal Scene Outliner lifecycle. Index adds one missing concern: persistent manual sibling ordering for actor and folder rows.

The plugin is Editor-only and operates on standard Actor Browser World Outliners.

## Persistence model

Index stores a single serialized hierarchy snapshot per `ULevel` in package metadata using the key:

```text
Index.Hierarchy.V1
```

The hierarchy is represented as ordered child lists rather than independent numeric values on every actor and folder.

Stable row identities are based on:

- `AActor::GetActorGuid()` for actors;
- `FFolder::GetActorFolderGuid()` for persistent folders when available;
- stable parent identifiers for actor parents, folder parents, and the level/root context.

The serialized value is compact internal text stored through `FMetaData`. It is not a separate asset, sidecar file, or Content Browser object.

Relevant Unreal APIs include:

- `UPackage::GetMetaData()` and `FMetaData`
- `AActor::GetActorGuid()`
- `FFolder::GetActorFolderGuid()`
- `FFolder::GetParent()`
- `FFolder::GetRootObject()`
- `FFolder::GetRootObjectAssociatedLevel()`
- `FActorFolders` public folder enumeration and lifecycle APIs

## Native World Outliner integration

Index registers a visually hidden Scene Outliner column through `FSceneOutlinerModule::OnCreateActorBrowserColumns()`.

`FIndexColumn` has two responsibilities:

1. Register the containing `ISceneOutliner` with Index so it can be refreshed when ordering changes.
2. Provide the manual sibling sort through `FIndexColumn::SortItems()`.

The native Item Label column remains the visible hierarchy UI. `ConstructRowWidget()` returns `SNullWidget`, so the Index column contributes no visible row content and no row-height behavior.

`SortItems()` receives one sibling list at a time. `FIndexSequenceState::EnsureCaptured()` reconciles that list against the saved hierarchy snapshot, then Index sorts the rows using the persisted sibling sequence.

## Sequence state

`FIndexSequenceState` owns the hierarchy-order data and the rules used to reconcile it with Unreal's current hierarchy.

The major responsibilities are:

- resolve stable actor and folder IDs;
- resolve the logical parent/group for each row;
- capture previously unseen sibling tiers;
- preserve actor/folder interleaving within a tier;
- move ordered blocks before or after a sibling;
- move rows into actor or folder parents;
- remove deleted IDs without disturbing remaining sibling order;
- remap identities where Unreal rebuilds folder rows;
- serialize and deserialize the level metadata snapshot;
- create transaction changes for Index metadata;
- reconcile native hierarchy moves and Undo/Redo restoration.

## Native hierarchy reconciliation

Unreal may move selected actors and folders incrementally, with Scene Outliner refreshes occurring between individual native callbacks. Treating each callback as an independent insert-at-top operation would reverse multi-item selections.

Index avoids that by reconciling sibling tiers from stable pre-move information rather than eagerly rewriting order inside actor/folder change callbacks.

The reconciliation system uses several complementary sources of ordering information:

- a short-lived immutable pre-move hierarchy baseline for native multi-item moves;
- historical visual paths for tracked rows;
- transaction-restored hierarchy snapshots during Undo/Redo;
- exact departure-tier snapshots captured before a tracked row leaves its source parent.

The exact departure-tier record is especially important for mixed operations. If a source tier temporarily compresses from `[G,H]` to `[H]`, later per-item indices are no longer sufficient to prove that G originally preceded H. Index therefore records the complete persisted source sequence before removal. When Undo returns the complete membership to that source parent, the recorded tier can be restored atomically.

Reconciliation is skipped while `GIsTransacting` so Index does not persist partially restored native hierarchy states while Unreal is actively applying Undo or Redo.

## Undo and Redo

`FMetaData` does not automatically participate in UObject transactions. Index therefore records explicit hierarchy metadata changes through Unreal's transaction system using `FCommandChange` snapshots.

The transaction layer restores Index metadata alongside Unreal's native hierarchy changes. After Undo or Redo completes, Index invalidates its cached sequence state and refreshes registered World Outliners so the visible tree is rebuilt from the settled hierarchy and restored ordering data.

Native hierarchy operations can span several internal editor transactions or deferred Outliner refreshes. Index's reconciliation data is designed to preserve the original sibling sequence across those intermediate states rather than depending on one particular callback or frame boundary.

## Drag and drop

Index does not replace `FActorBrowsingMode` and does not create a custom Outliner.

An `IInputProcessor` observes stock Scene Outliner drag operations and resolves Index-specific manual ordering behavior:

- top and bottom edge zones are explicit before/after insertions;
- the center remains a hierarchy operation;
- holding the configured force-reorder modifier expands before/after ordering across the full target row;
- blank-space drops can move the selected root block to the level/root tier;
- unsupported Scene Outliner item types are never claimed.

The validated target is cached during hover so mouse release commits the operation that was previewed.

Multi-selection is reduced to the highest selected roots before ordering. Their current visual hierarchy paths determine block order, so descendants already carried by a selected parent are not flattened or processed twice.

## Hierarchy operations

Index uses Unreal's hierarchy rules and adds explicit behavior only where manual ordering requires it.

Supported structural cases include:

- actor parenting;
- moving actors into folders;
- moving folders into folders;
- moving selected hierarchy roots as one ordered block;
- detaching actor children from their current actor parent;
- moving actor or folder children out of their current folder parent;
- rejecting actor attachment cycles;
- rejecting folder moves into themselves or their descendants.

Unreal remains authoritative for the actual actor attachment and folder hierarchy. Index updates the saved sibling sequence to match the resulting structure.

## Folder creation

Index intentionally does not subscribe to folder-created events or force an additional Outliner refresh when Unreal creates a folder.

`SSceneOutliner::CreateFolder()` and `FActorBrowsingMode::CreateNewFolder()` retain ownership of:

- creating the folder;
- moving the current selection;
- selecting the result;
- scrolling to it;
- activating inline rename;
- handling Enter, Escape, and normal keyboard focus.

Index reconciles the resulting sibling tiers through its normal hidden sort path after Unreal updates the hierarchy.

## Duplication

Index observes Unreal's editor duplication lifecycle so duplicates can be placed relative to their source block.

When the selection contains a folder, Ctrl+D is routed through Unreal's hierarchy-aware folder duplication path so contained actors and subfolders are duplicated together. Actor-only duplication remains Unreal's normal actor duplicate operation.

The selected highest roots are captured before duplication. Once Unreal has rebuilt the duplicate rows, Index resolves the resulting roots and places the duplicate block immediately after its source when that relationship is unambiguous.

Because Unreal's duplicate action and Index's final manual placement are distinct editor changes, they can appear as separate Undo/Redo steps.

## Reorder feedback

Index uses an input-transparent Slate popup for the before/after insertion line. The line is positioned at the target row boundary and spans the World Outliner tree viewport.

Index does not replace the native `STableRow` border brush or background color. This keeps Unreal's normal selection, hover, focus, and rename visuals intact.

Hierarchy actions use the existing decorated drag/drop tooltip APIs to communicate the resolved parent, move, detach, or validation result.

## Project settings

`UIndexSettings` is exposed under:

```text
Project Settings > Plugins > Index
```

The settings currently control:

- custom ordering enable/disable;
- Above insertion-line color;
- Below insertion-line color;
- row-edge reorder-zone percentage;
- force-reorder modifier key.

Index intentionally does not alter World Outliner row height or add parent/deparent row highlight styling.

## Native Scene Outliner source reference

The implementation was compared against Epic's Unreal Engine 5.8 `SceneOutliner` source supplied with the engine, especially:

- `Private/SSceneOutliner.cpp`
- `Private/SOutlinerTreeView.cpp`
- `Private/ActorBrowsingMode.cpp`
- `Private/ActorMode.cpp`
- `Public/ISceneOutliner.h`
- `Public/ISceneOutlinerMode.h`

These sources establish how the stock Outliner constructs actor-browser columns, routes drag/drop through its active mode, creates folders, manages inline rename, duplicates folder hierarchies, and refreshes tree rows.

The stock World Outliner does not expose a public persistent before/after sibling-ordering feature for existing rows, so Index layers that behavior over the standard Outliner instead of replacing the mode or modifying engine source.

## Motion Design / Avalanche reference

Epic's Motion Design `AvalancheOutliner` source was also useful as a reference for explicit tree-row insertion behavior, especially:

- `Private/Slate/SAvaOutlinerTreeRow.cpp`
- `Private/AvaOutlinerView.cpp`
- `Private/DragDropOps/AvaOutlinerItemDragDropOp.cpp`
- `Private/DragDropOps/Handlers/AvaOutlinerActorDropHandler.cpp`
- `Private/AvaOutlinerStyle.cpp`

Avalanche owns custom rows and can bind row-level drop handlers directly. Index deliberately keeps the standard World Outliner and implements only the manual ordering layer that stock Unreal does not provide.

## Plugin source map

- `Source/Index/Public/IndexModule.h` and `Private/IndexModule.cpp`: module startup/shutdown, Outliner column registration, editor delegates, duplication hooks, Undo client integration.
- `Source/Index/Public/IndexColumn.h` and `Private/IndexColumn.cpp`: hidden Scene Outliner sort column and Outliner registration.
- `Source/Index/Private/IndexSequenceState.h/.cpp`: persistence, stable identities, hierarchy reconciliation, ordering operations, transactions, Undo/Redo restore data.
- `Source/Index/Private/IndexInteraction.h/.cpp`: Scene Outliner drag/drop observation, target resolution, hierarchy actions, tooltips, input processing, refresh coordination.
- `Source/Index/Private/IndexLineOverlay.h/.cpp`: full-width before/after insertion-line Slate overlay.
- `Source/Index/Public/IndexSettings.h` and `Private/IndexSettings.cpp`: Project Settings configuration.
- `Source/Index/Private/Tests/IndexHierarchyState.spec.cpp`: automation coverage for hierarchy-order state and reconciliation behavior.
