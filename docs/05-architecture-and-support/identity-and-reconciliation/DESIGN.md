# Stable Identity and Reconciliation — Design

**Index v1.0.0 · User guide**

## Why labels can change without shifting order

Index recognizes Actors and Folders by stable identities rather than relying on their displayed names for ordering. That means you can rename “A_001” to “GameplayStart” without automatically moving it to another position.

## The parent tier matters

Every Level root, Folder and attached Actor parent has its own ordered list of compatible children. Reordering `Environment/Props` doesn't require rebuilding the root sequence or other Folders.

<figure class="doc-media" data-media-id="identity-parent-tiers" data-kind="image"><img src="../../assets/media-placeholder.png" alt="Diagram of parent-specific ordered Actor and Folder sequences" loading="lazy"><figcaption>Each Level or hierarchy parent maintains its own manual child sequence, independent of Actor and Folder labels.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Make a hierarchy diagram with a Level root containing mixed Actor and Folder children, an expanded Environment Folder with separately ordered children, and one attached Actor subtree. Indicate GUID-based identities rather than alphabetical names.</span></figcaption></figure>

## When Unreal changes several rows at once

Operations such as moving several Actors to a Folder may generate multiple native callbacks. Index attempts to preserve the **order from before the move**, rather than treating each incoming callback as a completely new Actor at the top.

## Why this matters for Undo

A parent Folder may temporarily have fewer children during a move. Index retains enough historical order information to recover the appropriate sibling slots as members return on Undo. This is an implementation safeguard, not something users need to configure.

## Limits

The saved order depends on valid current Level/Actor/Folder identities. Index cannot conjure rows for unloaded Actors, and it isn't a guarantee that arbitrary external tools won't rewrite native relationships.

---

[Read Technical](TECHNICAL.md)
