# Saving and Restoring Manual Order — Design

**Index v1.0.0 · User guide**

## The order is saved with the Level

Index stores the manual World Outliner Actor/Folder sequence in **level package metadata**, not in a separate asset, sidecar file, or component. Save the affected Level using **File → Save** or **Save All**, then reopen it to retrieve the saved sequence.

## Persistence workflow

1. Reorder a few loaded Actor/Folder rows in the Outliner.
2. Confirm the insertion lines and resulting order.
3. Save the Level. Its package may need to be checked out in Perforce.
4. Switch away and reopen the Level to confirm its saved arrangement.

<figure class="doc-media" data-kind="gif" data-media-id="persist-reopen"><img src="../../assets/media-placeholder.png" alt="Saved Index sequence reappearing after reopening the level" loading="lazy"><figcaption>A saved manual Outliner order is restored when its Level is reopened.<span class="media-capture-instructions"><strong>Capture instructions · GIF:</strong> Record a distinct nonalphabetical Actor and Folder sequence, save the Level, switch away and reopen it, then show that the identical order remains. Include the Level name and Save operation.</span></figcaption></figure>

## Perforce and One File Per Actor

Because the hierarchy snapshot belongs to the **Level**, a reorder marks that package dirty. With One File Per Actor, Actor content may live in External Actor files while the Index sequence remains on the Level package. Do not assume an Actor checkout alone persists an Index reorder.

## When an order appears lost

Clear Outliner filters, check other active column sorting and confirm the Level was saved. An alternate visible sort isn't necessarily evidence that Index metadata was removed.

---

[Read Technical](TECHNICAL.md)
