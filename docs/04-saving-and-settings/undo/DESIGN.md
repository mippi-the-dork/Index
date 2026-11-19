# Undo, Redo and Hierarchy Restoration — Design

**Index v1.0.0 · User guide**

## Reverse a change with Unreal's commands

Use **Ctrl+Z** to undo a manual insertion and **Ctrl+Y** to redo it. Confirm the restored sibling order in the Outliner.

<figure class="doc-media" data-kind="gif" data-media-id="undo-reorder"><img src="../../assets/media-placeholder.png" alt="Index reorder being undone and redone" loading="lazy"><figcaption>Index ordering participates in Unreal's Undo and Redo workflow.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record moving an Actor block before another row, pressing Ctrl+Z to restore the original sequence, and Ctrl+Y to reinstate it. Show the same Outliner rows during each step.</span></figcaption></figure>

## Hierarchy changes can take multiple steps

An edge reorder is an explicit Index operation. A native Folder move, duplication or parenting change may span additional Unreal Editor transactions. You might need more than one Undo step to return both the **hierarchy relationship** and **manual placement** to their original state.

## Check after Undo

When undoing a native hierarchy move, verify that an Actor returned to its original parent **and** its original position among siblings. If a duplicate remains after one Undo, inspect whether that step only reversed its later ordering.

## Saving

Undo changes the current editor state; save once you have reached the intended final sequence. Do not assume the persistent Level package already reflects an unsaved Undo state.

---

[Read Technical](TECHNICAL.md)
