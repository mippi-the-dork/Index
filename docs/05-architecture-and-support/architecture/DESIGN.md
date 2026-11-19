# Editor Architecture — Design

**Index v1.0.0 · User guide**

## Index works inside the standard Outliner

Index doesn't open another window or replace Unreal's Actor Browsing mode. It adds manual sibling ordering behind the existing Actor/Folder rows, observes drag/drop, and displays a simple insertion line.

The normal Outliner still owns row selection, labels, Folder creation, native parenting and component trees.

## How a user action flows

A drag starts in the World Outliner. Index identifies a row edge, a valid native hierarchy action, or the blank root region. It previews the placement with a line or tooltip. Upon a valid release it updates the hierarchy/sequence and refreshes the affected Outliner windows.

<figure class="doc-media" data-media-id="architecture-overview" data-kind="image"><img src="../../assets/media-placeholder.png" alt="Diagram of Index Editor module connections to native Unreal Outliner and Level state" loading="lazy"><figcaption>Index separates Outliner rendering, input handling, sequence storage and saved Level metadata.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Create an architecture diagram showing the native Scene Outliner, the hidden Index sort column, the Slate input processor and insertion-line overlay, the sequence state, Unreal native hierarchy, and the Level package. Label read, reorder, and save arrows.</span></figcaption></figure>

## What remains native

The Actor's transform, attachment rules and the Level's Folder structure remain standard Unreal behavior. Index itself doesn't install runtime components or control gameplay, and it doesn't add visual data columns to Actor rows.

## When to use Technical

The Technical counterpart identifies the main C++ classes and why several separate systems are necessary for safe persistence, especially Undo and incremental native hierarchy changes.

---

[Read Technical](TECHNICAL.md)
