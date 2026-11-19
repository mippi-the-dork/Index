# Release from Parent — Design

**Index v1.0.0 · User guide**

## Drag a child onto its own parent to move out

Index provides a quick way to release an item from its immediate parent. Drop the child onto the **center** of its current parent. The child moves to the compatible higher tier, normally placed immediately after that former parent.

```text
Table
  Lamp
```

Becomes:

```text
Table
Lamp
```

The equivalent operation for nested Folders moves a child Folder out one tier and places it after its enclosing Folder.

<figure class="doc-media" data-media-id="release-from-parent" data-kind="gif"><img src="../../assets/media-placeholder.png" alt="Child Actor released from attachment and placed after former parent" loading="lazy"><figcaption>A child can be detached by dropping it onto its existing parent, moving it to the next appropriate hierarchy tier.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record an attached Actor being dropped onto the center of its current parent, show the tooltip and resulting outdented row immediately after the parent. Include a similar nested Folder example if possible.</span></figcaption></figure>

## Center versus edge

The center means **release** only when Index recognizes that target as the actual current parent. The top/bottom edges are still before/after insertion targets. Use the drag tooltip and insertion line to distinguish them.

## What doesn't qualify

A non-child cannot be “released” from an arbitrary Actor. Invalid level relationships and unloaded World Partition rows aren't valid drag operations. Undo the operation if it changes a relationship you meant to preserve.

---

For implementation, [read Technical](TECHNICAL.md).
