# 02. Ordering and Dragging

This section explains the core Index interaction: **drag an Actor or Folder, preview a valid target, then release to change its manual position**. Index uses native Unreal Outliner drag/drop, keeping hierarchy actions in the center and adding explicit insertion zones at the row edges.

## Understand the three behaviors

**Before** inserts above a target, **After** inserts below it, and **Center** performs the described native parenting/Folder action. The default edges are 10% of row height, or hold Alt to turn the entire upper/lower half into reorder zones.

## Work on meaningful sets

Index supports interleaved Actor/Folder siblings, independent nested ordering and multi-selection blocks. It preserves original selected order and avoids processing child rows twice when their parent is also selected.

## Topics

| Topic | What it explains | Design | Technical |
| --- | --- | --- | --- |
| **Before and After Drop Zones** | Edge targets, expanded parents and valid insertion | [Design](edge-zones/DESIGN.md) | [Technical](edge-zones/TECHNICAL.md) |
| **Force Reorder Modifier** | Alt/Shift/Ctrl and half-row zones | [Design](force-reorder/DESIGN.md) | [Technical](force-reorder/TECHNICAL.md) |
| **Insertion Lines and Drag Feedback** | Lines, tooltips and drag interpretation | [Design](drag-feedback/DESIGN.md) | [Technical](drag-feedback/TECHNICAL.md) |
| **Actors and Folders in One Order** | Interleaved sibling tiers and independent child order | [Design](mixed-siblings/DESIGN.md) | [Technical](mixed-siblings/TECHNICAL.md) |
| **Multi-Selection Reordering** | Block movement, visual order and descendants | [Design](multi-selection/DESIGN.md) | [Technical](multi-selection/TECHNICAL.md) |

**Next:** [Hierarchy and Item Lifecycle](../03-hierarchy-and-lifecycle/README.md) covers parenting, detaching, duplication and new content placement.
