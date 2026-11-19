# New Actors and Folders — Design

**Index v1.0.0 · User guide**

## New content starts at the top of its own tier

Index places newly created Actors and Folders at the beginning of the relevant local sibling order when it can identify them as new hierarchy content. Existing siblings keep their relative order.

For a root containing Environment, Gameplay and Lighting, a new root Folder can appear above all three. A new Actor created inside Gameplay belongs at the **top of Gameplay's own child tier**, not at the root.

<figure class="doc-media" data-media-id="new-items-top" data-kind="gif"><img src="../../assets/media-placeholder.png" alt="New Actors and Folders appearing above existing local siblings" loading="lazy"><figcaption>Newly created rows enter at the top of their local hierarchy while previously ordered siblings stay unchanged.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record a custom-ordered root, create a new Actor and show its top-of-tier placement. Then create a Folder inside an existing Folder and show the local insertion without changing the root order.</span></figcaption></figure>

## Create Folder with Selection

Unreal handles the native creation, selected-Actor move, scroll, and inline rename. Index does not replace that workflow with a separate editor tool.

## New versus duplicate

Duplicated content normally aims to appear after its source when the source relationship is clear. New unrelated content takes the top-of-tier path. See [Duplication](../duplication/DESIGN.md) for the exceptions.

## Saving

These ordering edits are stored in the level package; use **Save** or **Save All**. New Actors and Folders are still native Unreal objects and relationships, not Index assets.

---

For implementation, [read Technical](TECHNICAL.md).
