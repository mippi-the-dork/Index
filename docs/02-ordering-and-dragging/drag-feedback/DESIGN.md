# Insertion Lines and Drag Feedback — Design

**Index v1.0.0 · User guide**

## Know what will happen before you release

Index deliberately uses two feedback paths:

- **Before / After insertion:** a full-width line at the target boundary.
- **Hierarchy action:** Unreal's decorated drag tooltip describes parenting, Folder movement, release from a parent, or why an action is invalid.

The native Outliner row remains visually familiar; Index does not replace row highlighting with a custom third-party interaction style.

<figure class="doc-media" data-media-id="drag-feedback-examples" data-kind="image">
<img src="../../assets/media-placeholder.png" alt="Comparison of Index Before line, After line, and hierarchy tooltip" loading="lazy">
<figcaption>Insertion lines show explicit ordering; descriptive drag tooltips distinguish parenting and Folder movement from before/after placement.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture three real hover states in a composite: a blue Before line, a different-blue After line, and a center-row hierarchy tooltip such as Move into Environment. Make each target row label visible.</span></figcaption>
</figure>

## What the line means

The line is a preview of the *committed* insertion destination rather than a generic highlight. The default Above line is a light blue; Below is a deeper blue. Both are customizable in Project Settings.

If no insertion line appears, Index may be yielding to native hierarchy behavior, rejecting an unsupported item, or waiting for a valid target. Read the tooltip and adjust your target instead of releasing blindly.

## Common confusion: indentation

The same vertical boundary can belong to different hierarchy levels depending on expanded children. Read the line's placement and target context before assuming the move stays among root-level siblings.

## Keep feedback readable

You can select high-contrast insertion colors without changing Unreal's selection, hover or row brushes. That matters in large Outliners where several items are highlighted or selected.

---

[Technical implementation](TECHNICAL.md) · [Ordering and Dragging overview](../README.md)
