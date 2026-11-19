# 05. Architecture and Support

Index is deliberately **Editor-only** and operates on standard World Outliner Actor/Folder rows. This section explains how it integrates with Unreal, which behaviors it doesn't replace, and how to diagnose unexpected ordering.

## Two systems, one Outliner

Unreal owns Actor attachment, Folder membership, labels and Level structure. Index observes native Outliner input and adds a **parent-specific saved ordering layer**. Its hidden sort column displays no new row UI; insertion lines appear only during relevant drag gestures.

## Technical source map

- `IndexModule.cpp`: editor lifecycle, Outliner registration, Actor/Folder events, duplication and Undo notifications.
- `IndexColumn.cpp`: hidden sort column and per-sibling order.
- `IndexInteraction.cpp`: drag validation, target classification, modifier, line feedback and special hierarchy actions.
- `IndexSequenceState.cpp`: stable identity, native-move reconciliation, metadata serialization and transactions.
- `IndexSettings.h/.cpp`: Project Settings options.
- `Tests/IndexHierarchyState.spec.cpp`: authored automation coverage.

## Topics

| Topic | What it explains | Design | Technical |
| --- | --- | --- | --- |
| **Editor Architecture** | Native integration and responsibilities | [Design](architecture/DESIGN.md) | [Technical](architecture/TECHNICAL.md) |
| **Stable Identity and Reconciliation** | GUIDs, tiers and native move history | [Design](identity-and-reconciliation/DESIGN.md) | [Technical](identity-and-reconciliation/TECHNICAL.md) |
| **Supported Actions and Limitations** | Scope and exclusions | [Design](limitations/DESIGN.md) | [Technical](limitations/TECHNICAL.md) |
| **Troubleshooting and Support** | Symptoms, resolution and diagnostic path | [Design](troubleshooting/DESIGN.md) | [Technical](troubleshooting/TECHNICAL.md) |

For issues and feature requests, use [Index GitHub Issues](https://github.com/mippi-the-dork/Index/issues).
