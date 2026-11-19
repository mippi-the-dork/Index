# Multi-Selection Reordering — Design

**Index v1.0.0 · User guide**

## Move several rows as one block

You can select multiple Actors/Folders and drag them together to another compatible position. Index uses their existing **visual order** rather than the order in which you clicked them.

Example:

```text
A  [selected]
B
C  [selected]
D
E  [selected]
```

Drop that selection after D, and the result retains the A–C–E order within the moved block.

## Parent and descendant selection

If both a parent and a child are selected, Index moves the **highest selected root** and carries its descendants; the child isn't moved a second time or flattened.

<figure class="doc-media" data-media-id="multi-block-drag" data-kind="gif">
<img src="../../assets/media-placeholder.png" alt="Multi-selection drag moving several Outliner rows together in order" loading="lazy">
<figcaption>A multi-selection moves as one contiguous ordered block, preserving its previous relative order.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record selecting three nonadjacent rows including at least one Folder or attached parent, dragging the selected rows together, hovering an After insertion line, and dropping. Hold the camera steady on the Outliner so the preserved order is obvious.</span></figcaption>
</figure>

## Valid targets

The destination must be appropriate for the selected roots and their level/hierarchy relationships. Mixed-level selections or other incompatible targets may be rejected. Narrow edge drops handle ordering; center drops can move the selected items into a Folder or attach them where valid.

## Undo

Use Unreal's normal **Ctrl+Z** to reverse an unintended move. Some complex native hierarchy moves may occupy multiple transaction steps; inspect the resulting hierarchy before applying unrelated changes.

---

[Technical implementation](TECHNICAL.md) · [Ordering and Dragging overview](../README.md)
