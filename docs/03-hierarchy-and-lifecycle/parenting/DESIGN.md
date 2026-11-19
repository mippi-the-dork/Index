# Parenting and Moving Into Folders — Design

**Index v1.0.0 · User guide**

## Reordering isn't reparenting

Index preserves Unreal's **native hierarchy action** at the center of a target row. Dragging an Actor onto the center of another Actor can attach it as a child; dragging onto a Folder center can move compatible content into that Folder. The drag tooltip describes the intended operation.

By contrast, dropping at a **top/bottom edge** changes manual sibling order. The Force Reorder modifier turns the full upper/lower half into an ordering target.

## Workflow: place a Lamp beneath Table

Drag `Lamp` onto the **center** of `Table`, inspect the parenting tooltip, and release. The Lamp becomes a child of Table. To put Lamp below Table **without attachment**, Undo and use the lower edge insertion line instead.

<figure class="doc-media" data-media-id="parent-vs-insert" data-kind="gif"><img src="../../assets/media-placeholder.png" alt="Comparison of native Actor parenting and Index sibling insertion" loading="lazy"><figcaption>Dropping on an Actor center changes parenting, while dropping at the row edge only changes ordering.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record two operations: drop a Lamp Actor on the center of Table and show the child attachment; undo; then drop Lamp at the lower edge of Table, showing the insertion line and unchanged parent relationship.</span></figcaption></figure>

## Moving into Folders

Drag a compatible Actor or Folder onto the center of `Environment`. Moving the parent Folder carries its nested children; moving an attached parent Actor carries its descendants. Index does not flatten either structure.

## What is refused

Unreal and Index reject invalid attachment cycles, Folder moves into themselves/descendants, unsupported row types and incompatible cross-level moves. If the drag doesn't show a valid target, do not assume releasing will reorganize the hierarchy.

---

For implementation, [read Technical](TECHNICAL.md).
