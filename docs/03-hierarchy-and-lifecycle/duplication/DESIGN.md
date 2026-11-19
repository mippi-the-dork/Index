# Duplicating Actors and Folders — Design

**Index v1.0.0 · User guide**

## Copies usually appear beside the source

When the source relationship is unambiguous, Index places a duplicated Actor/Folder block **immediately after** the original in that local sibling tier.

```text
Prop_A
Prop_B
Prop_B_Copy
Prop_C
```

Nested Folder relationships inside the duplicated block remain intact; Index doesn't flatten the result.

<figure class="doc-media" data-media-id="duplicate-beside-source" data-kind="gif"><img src="../../assets/media-placeholder.png" alt="Actor and Folder duplicates adjacent to their originals" loading="lazy"><figcaption>A duplicate block is placed after its original when both belong to the same clearly identified sibling tier.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record Ctrl+D on a single Actor and show the duplicate directly below it. Then duplicate a Folder containing nested Actors and show the copied Folder, subtree and resulting adjacent order.</span></figcaption></figure>

## Multi-selection and exceptions

Index attempts to identify the **highest selected roots** and preserve their visual order. If the selection spans incompatible or ambiguous tiers, placement can fall back to the new-item rule—at the top of each resulting tier.

## Undo can take multiple steps

The native Unreal duplication and Index's later placement may be separate editor transactions. Inspect the hierarchy after each Undo or Redo, rather than assuming one shortcut always removes the copy and restores all ordering.

## Workflow

Select an Actor/Folder, use the native duplicate command, inspect the resulting placement, and save the affected level after confirming the desired order.

---

For implementation, [read Technical](TECHNICAL.md).
