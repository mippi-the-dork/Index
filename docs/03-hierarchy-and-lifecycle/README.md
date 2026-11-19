# 03. Hierarchy and Item Lifecycle

Index works with **Unreal's real hierarchy**, not an invented flat list. It distinguishes changes to **parent relationships** from changes to **sibling ordering**, while adapting when Actors and Folders are created, duplicated, renamed, moved or removed.

## Read the drag destination

Dropping on the **center** of a row can attach/move the item into that parent. Dropping at an **edge** requests a pure Before/After insertion. Dropping a child onto its *current* parent center can release it to a higher tier. Blank Outliner space can move a block to root.

## What happens to existing ordering?

New content begins at the top of its local tier; duplicates normally appear after their source when unambiguous; renames and filters preserve the stored sequence; and deleted items are removed without resorting the remaining siblings. Native Undo/Redo can return earlier hierarchies.

## Topics

| Topic | What it explains | Design | Technical |
| --- | --- | --- | --- |
| **Parenting and Moving Into Folders** | Center drops and native hierarchy behavior | [Design](parenting/DESIGN.md) | [Technical](parenting/TECHNICAL.md) |
| **Release from Parent** | Actor detach and Folder outdent | [Design](releasing-parents/DESIGN.md) | [Technical](releasing-parents/TECHNICAL.md) |
| **New Actors and Folders** | Top-of-tier placement | [Design](new-items/DESIGN.md) | [Technical](new-items/TECHNICAL.md) |
| **Duplicating Actors and Folders** | Adjacent copies and ambiguous tiers | [Design](duplication/DESIGN.md) | [Technical](duplication/TECHNICAL.md) |
| **Renaming, Filtering and Native Sorting** | Stable IDs and display-order differences | [Design](rename-filter-sort/DESIGN.md) | [Technical](rename-filter-sort/TECHNICAL.md) |
| **Dropping into Empty Outliner Space** | Root placement at the bottom | [Design](blank-space/DESIGN.md) | [Technical](blank-space/TECHNICAL.md) |
| **Deleting and Removing Items** | Stable remaining order and Undo | [Design](deletion/DESIGN.md) | [Technical](deletion/TECHNICAL.md) |

Next: [Persistence and Preferences](../04-saving-and-settings/README.md).
