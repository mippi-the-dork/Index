# Troubleshooting and Support — Design

**Index v1.0.0 · User guide**

## I keep parenting instead of reordering

Aim at the upper/lower **edge** of the destination row, or hold **Alt** (the default Force Reorder modifier) to use the entire upper/lower half. Wait for the colored insertion line; the center area intentionally permits hierarchy operations.

## Nothing moves when I drop

Confirm **Project Settings → Plugins → Index → Enable Custom Ordering** is enabled. Check that you're using the normal World Outliner and a valid loaded Actor or Folder. An unsupported target or different Level context can reject insertion.

<figure class="doc-media" data-media-id="troubleshooting-edge" data-kind="image"><img src="../../assets/media-placeholder.png" alt="Outliner drag showing hierarchy tooltip versus valid Index insertion line" loading="lazy"><figcaption>A visible insertion line confirms an explicit reorder target, helping distinguish it from parenting or invalid drops.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture two views of the same drag: the center of a Folder row with a native Move into Folder tooltip and the top edge showing an Index Before insertion line. Include pointer positions and readable tooltips.</span></figcaption></figure>

## My order disappeared

Clear Outliner search filters. Check if another column is controlling visible sorting. Perform a small Index reorder to restore the manual sequence. Confirm you saved the affected **Level**, not just an External Actor package.

## A duplicate isn't beside the original

Index places copies after their source when the source/destination tier is unambiguous. Mixed-level or ambiguous multi-selection can fall back to top-of-tier placement.

## Undo doesn't immediately remove the duplicate

Native duplication and final Index sequence placement can be separate transactions. Inspect the hierarchy between Undo steps.

## A World Partition Actor can't be moved

Load it so it is represented by an editable row in the standard World Outliner.

## Report a reproducible issue

Use [Index GitHub Issues](https://github.com/mippi-the-dork/Index/issues). Include the Index and exact Unreal versions, installation method, Level and World Partition/OFPA state, hierarchy before and after, what row(s) you dragged, target zone, modifier, expected/actual result, screenshots/GIF and Output Log excerpts.

---

[Read Technical](TECHNICAL.md)
