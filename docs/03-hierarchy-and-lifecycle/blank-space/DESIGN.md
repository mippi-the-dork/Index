# Dropping into Empty Outliner Space — Design

**Index v1.0.0 · User guide**

## Move a block to the root

Dragging selected loaded Actors or Folders into the **blank area beneath the World Outliner rows** is an explicit root-move gesture where supported.

Index takes the selected highest hierarchy roots, moves them to the level's compatible root context and places the resulting block **at the bottom** of the root sibling sequence.

<figure class="doc-media" data-media-id="blank-space-root" data-kind="gif"><img src="../../assets/media-placeholder.png" alt="Dragging a nested Actor into the empty Outliner area to move it to root" loading="lazy"><figcaption>A blank-space drop moves the selected hierarchy roots into the level root and places the block after existing root items.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record an expanded Folder containing a child Actor, drag the child into the empty Outliner region below all listed rows, show the root-level insertion indicator, and release to reveal the Actor at the bottom of the level root. Keep descendants visible if moving a parent block.</span></figcaption></figure>

## What travels with the selection

A parent Actor carries its attached descendants; a Folder carries its nested subtree. Descendants aren't flattened or separately inserted.

## Example

A gameplay Actor under `Encounters` can be brought to root without finding a separate root row as the drop target. Verify its new parent and hierarchy level afterward.

## Limits

Empty-space moves remain subject to compatible level/root constraints. Index doesn't move items from one level to another just because the cursor is outside the row area. Dragging into unrelated editor panels isn't the same as the Outliner's recognized empty region.

---

[Technical implementation](TECHNICAL.md).
