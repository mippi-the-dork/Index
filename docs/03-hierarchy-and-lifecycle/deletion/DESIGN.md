# Deleting and Removing Items — Design

**Index v1.0.0 · User guide**

## Delete without resorting everything else

When an Actor or Folder is removed through Unreal's normal deletion workflow, Index removes its recorded identity from the hierarchy sequence. **The remaining siblings keep their relative order**.

```text
A
B
C
D
```

Deleting B leaves A, C, D in the same relative order. Index does not alphabetically rebuild the tier.

<figure class="doc-media" data-media-id="deletion-sequence" data-kind="image"><img src="../../assets/media-placeholder.png" alt="Before and after hierarchy showing stable sibling order after deletion" loading="lazy"><figcaption>Removing an Actor or Folder leaves the remaining manually ordered siblings in their previous relative positions.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture a two-panel Before/After comparison of a custom ordered Outliner where a middle Actor has been deleted. Keep the remaining item names identical and show their order has not shifted beyond closing the removed slot.</span></figcaption></figure>

## Delete safely

Use Unreal's standard Actor or Folder delete commands. If deleting a Folder also affects Actors inside it, that relationship follows Unreal's native Folder deletion behavior; Index is not a recovery tool for deleted content.

## Undo and recovery

Use Unreal's Undo immediately if the removal was accidental, then inspect both the restored item and the manual sibling placement. For complex Folder changes, additional Undo steps may be involved.

## Empty lists

A Folder may lose its last child and later receive new content; Index can reconcile its local tier when that content is next visible. The saved manual hierarchy isn't a global alphabetic sorting rule.

---

[Technical implementation](TECHNICAL.md).
