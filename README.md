# Index

Index adds persistent manual hierarchy ordering to Unreal Engine's standard World Outliner. Actors and folders share the same ordered sibling lists, so the Outliner stays where you put things instead of falling back to label-based ordering.

**Compatibility:** Unreal Engine 5.8.2 on Windows 64-bit.

Index is an Editor-only plugin. It requires no engine modifications and has no dependency on Motion Design or other third-party plugins.

## Features

- Reorder actors and folders directly in the native World Outliner.
- Interleave actors and folders in any sibling tier, such as `Actor A -> Folder -> Actor B`.
- Preserve manual position when actors or folders are renamed.
- Move multiple selected items as one ordered block while preserving nested relationships.
- Preserve Unreal hierarchy semantics by reducing structural moves to the highest selected roots.
- Drop a child onto its current parent to release it from that relationship.
- Insert newly created actors and folders at the top of their local hierarchy tier.
- Place duplicated actor and folder blocks immediately after their source block when the source relationship is unambiguous.
- Preserve ordering through level saves, editor restarts, level switching, filtering, and multiple World Outliner panels.
- Support Undo and Redo for ordering and hierarchy operations.
- Show contextual drag text describing the action that will occur on release.
- Show full-width before/after insertion lines for exact reorder placement.
- Configure reorder colors, edge drop-zone size, and the force-reorder modifier.
- Add no runtime actors, components, assets, or packaged-game dependency.

## Installation

### From a release ZIP

1. Close Unreal Editor.
2. If Index is already installed, delete the existing `YourProject/Plugins/Index` folder rather than merging files.
3. Extract the `Index` folder into your project's `Plugins` folder.
4. Confirm the descriptor is at `YourProject/Plugins/Index/Index.uplugin`.
5. Open the project and enable **Index** under **Edit > Plugins**.
6. Restart the editor if prompted.

Use a package built for the matching Unreal Engine version and platform. GitHub's automatically generated source archives are not precompiled plugin packages.

### From source

1. Place the `Index` folder in your C++ project's `Plugins` folder and close Unreal Editor.
2. Right-click the `.uproject` file and choose **Generate Visual Studio project files**.
3. Open the solution and build the project's **Development Editor / Win64** target.
4. Open Unreal, enable Index, and restart if prompted.

## Reordering

Index divides compatible World Outliner rows into three drop regions:

| Drop position | Result |
| --- | --- |
| Top edge | Insert the moved block immediately **before** the resolved sibling. |
| Center | Use the hierarchy action described by the tooltip, such as parent, move into folder, or detach. |
| Bottom edge | Insert the moved block immediately **after** the resolved sibling. |

For before/after drops, Index draws a full-width insertion line across the World Outliner tree. The Above and Below line colors can be configured independently in Project Settings.

If no Index insertion line is visible, the center hierarchy operation described by the tooltip remains in control. Supported actor/folder structural drops are applied using the highest selected roots so descendants already carried by a selected parent are not flattened or processed twice.

### Force reorder modifier

Hold **Alt** by default to turn the full target row into two reorder zones:

- Top half inserts before.
- Bottom half inserts after.

The modifier can be changed to **Shift**, **Ctrl**, or disabled under **Project Settings > Plugins > Index**.

## Ordering rules

Index treats actors and folders identically wherever Unreal's hierarchy allows them to share a sibling tier.

### Rename

Renaming an actor or folder changes only its label or path. It does not change its saved sibling position. Rename feedback remains Unreal's native World Outliner behavior.

### New content

A newly created actor or folder enters at the **top of its local hierarchy tier**. Existing sibling order remains unchanged.

When Unreal creates a folder containing the current selection, Index leaves Unreal's membership and destination rules untouched while preserving the selected rows' pre-move visual traversal order. This prevents sequential native hierarchy callbacks from reversing multi-item selections.

### Empty-space drop

Dropping an actor/folder block into blank space below the World Outliner rows moves the highest selected roots to the level/root tier and places the block at the bottom. Nested descendants remain beneath their moved parent and unrelated siblings are not absorbed into the moved block.

### Duplicate

A duplicated actor, folder, or selected hierarchy block is placed immediately after its source block when Index can resolve a clear local source relationship. Descendants remain beneath their duplicated highest selected parent rather than being flattened into the destination tier.

When the Outliner selection contains a folder, **Ctrl+D uses Unreal's hierarchy-aware folder duplication path**, so contained actors and subfolders are duplicated with it. Actor-only Ctrl+D continues through Unreal's normal duplicate command.

If the source relationship is ambiguous across multiple hierarchy tiers, duplicated roots use the normal new-content rule for their resulting tier.

### Multi-selection

When multiple rows are dragged, Index collapses the selection to its highest selected roots. Descendants already carried by a selected parent are not processed again.

The remaining roots preserve their current visual order and become one contiguous block at the destination.

For example:

```text
A   selected
B
C   selected
D
E   selected
```

Dropping the selection after `D` produces:

```text
B
D
A
C
E
```

### Delete

Deleting an item removes that row from the saved child list. Remaining siblings retain their relative order without a separate renumbering pass.

## Hierarchy behavior

Unreal continues to own the actual actor attachment and folder hierarchy. Index owns the order of children beneath each hierarchy node.

Example:

```text
ROOT
|- Actor A
|- Folder X
|  |- Actor C
|  |- Folder Y
|  `- Actor D
|- Actor B
`- Folder Z
```

Each parent has an explicit ordered child list. Renaming an item does not rebuild those lists.

### Parenting and folders

Center-dropping onto an actor or folder uses the hierarchy action reported by the drag tooltip. Index then records the resulting children in their new local tier.

- Moving into a new parent or folder inserts the incoming block at the top unless an explicit before/after insertion was requested.
- Moving a folder carries its entire subtree without changing the internal order of that subtree.
- Moving a parent actor carries attached descendants selected through that parent.
- Index rejects moves that would create an actor attachment cycle or move a folder inside itself or one of its descendants.
- Custom hierarchy moves remain level-scoped and root-compatible.

### Drop onto current parent

Dropping a child onto its current parent means **release from that parent**.

- Actor child onto current actor parent: detach and place immediately after the former parent in the resulting sibling tier.
- Actor already directly inside its current folder onto that folder: move out one folder level and place immediately after the former folder.
- Child folder onto its current parent folder: move out one folder level and place immediately after the former folder.
- Attached actor onto its containing folder: detach the actor attachment and keep or move the actor into that folder as described by the tooltip.

## Drag feedback

Index keeps its visual feedback limited to manual ordering:

- **Reorder:** a full-width insertion line marks the exact destination gap. Above and Below can use different colors.
- **Hierarchy action:** the drag decorator describes the resolved parent, move, or detach action.
- **Rename:** Unreal provides the native World Outliner rename feedback.

Examples of drag text include:

- `Reorder Cube_A before Folder_Props`
- `Reorder 3 items after Platform`
- `Parent Lamp to Table`
- `Move 2 items into Environment`
- `Detach from Parent_A and place after it`
- `Move folder out of Environment and place after it`
- native validation text when Unreal rejects an operation

Index does not replace native World Outliner row styling and does not alter row height.

## Project settings

Open **Project Settings > Plugins > Index** to customize:

| Setting | Effect |
| --- | --- |
| Enable Custom Ordering | Enables Index drag/drop ordering behavior. |
| Above Line Color | Color used when inserting above a row or gap. Defaults to a lighter placement blue. |
| Below Line Color | Color used when inserting below a row or gap. Defaults to a deeper placement blue. |
| Edge Threshold (%) | Percentage of the top and bottom of a row that acts as a reorder zone. |
| Force Reorder Modifier Key | Modifier that turns the full row into Before/After reorder zones. |

## Search and filtering

Outliner filtering changes visibility, not saved hierarchy. Index applies a reorder to the complete underlying sibling list relative to the visible target. Rows hidden by the current filter are not silently added to the moved block.

## Saving and persistence

Index does **not** create a separate asset or sidecar file.

Each level stores one serialized Index hierarchy snapshot in editor package metadata under:

```text
Index.Hierarchy.V1
```

The value contains ordered child lists keyed by stable row identity:

- actors use their editor Actor GUID;
- folders use their Actor Folder GUID when available;
- parent lists identify actor parents, folder parents, or the level/root context.

The serialized value is an internal compact text format stored directly in the level package metadata. It is not a JSON asset and does not create a new Content Browser object.

Save the affected level normally after changing Index order. The hierarchy metadata is saved with that level and survives editor restarts.

Because the complete ordering snapshot belongs to the level, an Index ordering change dirties the level package rather than writing independent sequence values into every actor and folder package.

## Undo and Redo

`FMetaData` is not automatically transactional, so Index records explicit hierarchy snapshots through Unreal's transaction system and reconciles them with native actor/folder hierarchy changes.

For native multi-item operations, Index preserves pre-move ordering, tracks exact source sibling tiers for departing rows, and restores complete source sequences when Undo returns their membership. This prevents temporary hierarchy compression from reversing sibling order during mixed actor/folder operations.

After Undo or Redo, Index invalidates its cached state and refreshes registered World Outliners from the restored hierarchy metadata.

## Outliner integration

Index registers a visually hidden sorting column in standard Actor Browser World Outliners. The column provides the saved manual sequence while Unreal's native Item Label column remains the visible hierarchy UI.

The hidden Index row widget contributes no visual content and no row sizing. It exists only to supply manual sort order and register the Outliner with Index.

Sorting the Outliner by another visible column can temporarily display a different presentation order. Performing an Index reorder reactivates the saved manual sequence.

## Scope and limitations

- Index targets loaded actor and persistent folder rows in standard Editor World Outliners.
- PIE, simulation worlds, runtime gameplay, Blueprint class editors, and specialized custom Outliners are outside its editing scope.
- Multiple standard World Outliner windows are supported and refreshed together.
- Actors and folders can share one manual sequence when they are valid siblings under Unreal's hierarchy rules.
- Attached actor child tiers contain actors only because folders cannot be attached beneath actors.
- Index does not custom-reorder items across different levels.
- Unloaded World Partition actors cannot be reordered until they are represented by loaded Outliner rows.
- Component rows and unsupported Scene Outliner item types remain native and are not claimed by Index.
- Other plugins that replace or intercept the same World Outliner drag/drop or global Slate input behavior may require compatibility testing.
- Ctrl+D duplication can use separate Undo/Redo steps for Unreal's duplicate action and Index's final placement.
- Index is Editor-only and adds no runtime gameplay classes.

See [Doc/Sources.md](Doc/Sources.md) for implementation architecture and Unreal Engine source references.
