# 04. Saving and Settings

Index stores **the manual sequence in the Level package** and the controls for its editor interaction under **Project Settings → Plugins → Index**. This distinction matters when saving under Perforce or One File Per Actor.

## How persistence works

A successful reorder updates `Index.Hierarchy.V1` in Level metadata and marks the Level package dirty. Save the Level to preserve the order. Undo and Redo integrate with native editor transactions, although complex native hierarchy operations may use multiple steps.

## Topics

| Topic | What it explains | Design | Technical |
| --- | --- | --- | --- |
| **Saving and Restoring Manual Order** | Level metadata and source control | [Design](persistence/DESIGN.md) | [Technical](persistence/TECHNICAL.md) |
| **Undo, Redo and Hierarchy Restoration** | Order history and native operations | [Design](undo/DESIGN.md) | [Technical](undo/TECHNICAL.md) |
| **Project Settings and Visual Feedback** | Thresholds, colors, modifiers | [Design](preferences/DESIGN.md) | [Technical](preferences/TECHNICAL.md) |
| **World Partition, Levels and Editor Scope** | Loaded rows and editor limits | [Design](compatibility/DESIGN.md) | [Technical](compatibility/TECHNICAL.md) |

Next: [Architecture and Support](../05-architecture-and-support/README.md).
