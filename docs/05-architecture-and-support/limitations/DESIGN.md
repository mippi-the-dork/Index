# Supported Actions and Limitations — Design

**Index v1.0.0 · User guide**

## What Index does

Index is a **manual Actor/Folder ordering tool in the standard Unreal Editor World Outliner**. It supports compatible before/after insertion, native parenting/Folder movement, detach gestures, multi-selection, root placement, creation, duplication, Undo and saved per-Level ordering.

## What Index deliberately doesn't do

| Outside its scope | Reason |
| --- | --- |
| Automatically alphabetize/reclassify all content | Manual order is chosen by the user |
| Change 3D Actor transforms or gameplay | Index only owns editor hierarchy organization |
| Reorder Blueprint components | Component rows are outside its Actor/Folder ordering path |
| Reorder unloaded World Partition Actors | No loaded Actor row is available for the drag |
| Reorder across separate Levels | Per-Level sequence and native compatibility constraints |
| Replace Actor attachment or Folder relationships | Unreal remains authoritative |
| Work inside packaged games | The plugin is Editor-only |
| Force every custom Scene Outliner to participate | Integration targets standard Actor Browser Outliners |

<figure class="doc-media" data-media-id="limits-valid-drops" data-kind="image"><img src="../../assets/media-placeholder.png" alt="Supported Index World Outliner rows versus unsupported editor contexts" loading="lazy"><figcaption>Index only intercepts compatible loaded Actor and Folder interactions in the standard World Outliner.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Create a two-column comparison: on the left a standard World Outliner with loaded Actors and Folders and an insertion line, on the right a Blueprint Components tree or a clearly unloaded World Partition Actor that isn't a supported drag target. Label Supported and Not Supported without inventing controls.</span></figcaption></figure>

## Common misreadings

A changed visible order after sorting another column isn't necessarily a lost saved order. A center drop that changes parent isn't a pure reorder. Saving an Actor's external package alone doesn't necessarily persist the Level-owned sequence.

## Which workaround is appropriate?

Use native Unreal tools for unsupported hierarchy changes. For Level-local sequence issues, clear filters, use a valid Index drag and save the Level. For cross-Level content organization, use Unreal's ordinary Level management rather than expecting Index to merge sequences.

---

[Read Technical](TECHNICAL.md)
