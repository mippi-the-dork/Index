# World Partition, Levels and Editor Scope — Design

**Index v1.0.0 · User guide**

## Where Index operates

Index v1.0.0 targets the **standard Unreal World Outliner Actor Browser** in the Editor, on the plugin's Win64/UE 5.8 target. It manages **loaded Actor and Folder rows**; it does not change gameplay, Actor transforms, component ordering or runtime hierarchy behavior.

## World Partition

An unloaded World Partition Actor cannot be manually dragged as a loaded Outliner item. Load it before trying to place it in a different sibling slot. The Level still owns the saved manual sequence.

<figure class="doc-media" data-kind="image" data-media-id="wp-loaded-rows"><img src="../../assets/media-placeholder.png" alt="Loaded World Partition Actors in World Outliner compared with unloaded cells" loading="lazy"><figcaption>Index works on loaded Outliner rows, while unloaded World Partition content isn't an immediate drag target.<span class="media-capture-instructions"><strong>Capture instructions · Image:</strong> Capture a World Partition Level with loaded Actors visible in the Outliner and corresponding unloaded cells/Actors in the World Partition interface. Clearly distinguish loaded draggable rows from unloaded content.</span></figcaption></figure>

## Level boundaries

Index stores per-Level hierarchy sequences and does not introduce custom ordering between separate Levels. Unsupported item types, Blueprint component trees, PIE/Simulation runtime Outliners and specialized custom Outliners remain outside its editing scope.

## Multiple World Outliner windows

Index can refresh more than one registered standard World Outliner, but each window may temporarily display different native column sorting. The hierarchy metadata remains the same Level-owned source of manual order.

## Compatibility caution

The descriptor declares EngineVersion **5.8.0**, while the README lists 5.8.2 as the target. Use a binary release built for your engine and verify in your project; don't assume cross-version/platform support.

---

[Read Technical](TECHNICAL.md)
