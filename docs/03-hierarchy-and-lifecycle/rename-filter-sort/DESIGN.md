# Renaming, Filtering and Native Sorting — Design

**Index v1.0.0 · User guide**

## Names and order are independent

Renaming an Actor or Folder does not intentionally change its manual Index position. You can use descriptive names without artificial numeric prefixes.

For instance, a row named `PlayerStart` can become `BP_PlayerSpawn` and retain its position between Gameplay and Environment.

## Searching the Outliner

The World Outliner search/filter changes which rows are *visible*. It doesn't erase their stored sibling positions. Clearing the filter reveals the complete sequence again.

## Sorting by other columns

Clicking another Outliner column may temporarily display that column's sorting order. **Your saved Index sequence isn't necessarily gone.** Performing a valid Index reorder switches back to the manual ordering presentation.

<figure class="doc-media" data-media-id="rename-filter-sequence" data-kind="gif"><img src="../../assets/media-placeholder.png" alt="Animated demonstration of stable custom order across rename and filtering" loading="lazy"><figcaption>Renaming and filtering do not change the underlying saved manual sibling sequence.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record a custom-ordered Outliner, rename an Actor to something that would alphabetize differently, filter down to two rows, then clear the filter. Finally sort by another column and perform an Index reorder to restore the manual presentation.</span></figcaption></figure>

## If the order seems missing

First clear filters and check whether another column is actively sorted. Then perform a small valid Index insertion. If a saved sequence is genuinely lost after restart, confirm the relevant level package was saved rather than just the Actor.

---

[Technical implementation](TECHNICAL.md).
