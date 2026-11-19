# Actors and Folders in One Order — Design

**Index v1.0.0 · User guide**

## A shared sequence where Unreal permits it

Actors and Folders may appear together in one sibling tier. Index preserves the combined visible order; it doesn't first alphabetize every Folder and then every Actor.

For example, you can arrange the root as:

```text
PlayerStart
Environment/
DirectionalLight
Gameplay/
PostProcessVolume
```

Each Folder can have its **own internal order**, independent of the level root. Attached Actor children have their own parent Actor tier.

<figure class="doc-media" data-media-id="mixed-sibling-order" data-kind="image">
<img src="../../assets/media-placeholder.png" alt="Mixed Actor and Folder sibling order with an independent nested Folder order" loading="lazy">
<figcaption>Actors and Folders retain their chosen interleaved sequence within each hierarchy tier.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture the Outliner with Actors and Folders clearly interleaved at the root, plus one expanded Folder containing its own nonalphabetical internal order. Keep indentation and item icons readable.</span></figcaption>
</figure>

## Reordering one tier

Drag an Actor or Folder before another compatible sibling to change only that local sequence. Folder children travel with the parent Folder, but the child tier's saved order isn't flattened.

## Hierarchy restrictions

Only compatible sibling/parent contexts can be reordered. Unreal does not support Folders as attached Actor children, and Index doesn't override that rule. Moving between levels is not a supported custom reorder.

## Sorting another column

A temporary native Outliner column sort can change **presentation** without replacing the saved Index sequence. Return to Index ordering by performing an Index reorder in the appropriate Outliner.

---

[Technical implementation](TECHNICAL.md) · [Ordering and Dragging overview](../README.md)
