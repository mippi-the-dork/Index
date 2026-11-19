# Index Roadmap

**Current documented release:** Index v1.0.0 · Unreal Engine 5.8 · Win64 · Editor-only  
**Milestone status:** v1.0.0 released; future versions **not yet committed**  
**Source of current behavior:** [Index README](https://github.com/mippi-the-dork/Index/blob/main/README.md) and [Index implementation notes](https://github.com/mippi-the-dork/Index/blob/main/Doc/Sources.md)

## Release history and direction

| Stage | Status | Scope |
| --- | --- | --- |
| **v1.0.0 — Persistent Manual Ordering** | Released | Before/After insertion; mixed Actor/Folder siblings; multi-selection; hierarchy operations; new/duplicate placement; saved Level metadata; Undo/Redo; configurable drag feedback |
| **Future work** | **Not scheduled** | No approved version number, feature list or release date has been provided |

The documentation deliberately avoids inventing previous release milestones. The v1.0.0 list describes the current implementation rather than claiming each feature was added at a particular historical date.

## v1.0.0 — The current foundation

Index solves one specific problem: **the World Outliner should reflect the order a designer wants, independent of display names**.

The released scope includes:
- explicit Before/After insertion with full-width indicator lines;
- Force Reorder modifier and configurable edge thresholds;
- Actors and Folders sharing an ordered sibling tier;
- multi-selection blocks retaining visual order;
- native parenting, Folder movement and release-from-parent gestures;
- placement rules for new and duplicated hierarchy content;
- Level-owned `Index.Hierarchy.V1` metadata and Undo/Redo;
- native editor integration without runtime content, custom Actor types or engine modifications.

The current site documents those capabilities separately in [Design](02-ordering-and-dragging/README.md) and [Technical](05-architecture-and-support/README.md) guides.

## Future work — No committed version yet

The next release's features have **not been specified**. The following are product review questions, not scheduled features:

- Are there recurring Outliner organization workflows that the current manual insertion gestures do not adequately support?
- Which editor integration issues appear in large projects, multi-Level setups or World Partition, and which can be improved safely without modifying the engine?
- Are there usability improvements worth exploring for drag feedback, keyboard interaction, accessibility and large selections?
- Would more automated native editor tests or compatibility coverage be valuable before broadening feature scope?

Do not treat these questions as an announced v2.0 roadmap. Once actual features are chosen, this page should record the target milestone, intended behavior, supporting tests and release status.

## Release quality expectations

Any future Index release should preserve current manual-order semantics, native hierarchy relationships, per-Level package ownership, Undo/Redo correctness, existing insertion gestures and predictable Editor performance.

A release also needs updated **Design** and **Technical** pages, tested plugin compatibility, real editor screenshots/GIFs, and an explicit verification record.

## What isn't part of current Index

Automatic alphabetical sorting, custom runtime hierarchies, reordering unloaded World Partition rows, cross-Level custom ordering and editing Blueprint Component rows are **not shipped capabilities**. Their absence here does not imply a commitment to add them.

---

[Current documentation](README.md) · [Verification Matrix](VERIFICATION_MATRIX.md) · [Documentation Audit](DOCUMENTATION_AUDIT.md)
