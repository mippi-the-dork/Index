# Index Documentation Audit

**Baseline:** Index v1.0.0 · Public source, README and `Doc/Sources.md`  
**Review type:** Editorial and source inspection; not a completed Unreal Editor/browser test session.

## What the site now covers

Five documentation sections organize **23 topics and 46 paired Design/Technical pages**. Each topic explains a user workflow independently from its C++ implementation. Existing genuine Index screenshots and GIFs are used for reordering, Force Reorder, insertion indicators and preferences.

The additional diagrams and animations use a **neutral placeholder** until real Index screenshots/recordings are provided. Each placement has a reader-facing caption, an explicit Image/GIF type and a separate capture instruction.

## Priorities for final documentation quality

| Priority | Gap | Recommended follow-up | Status |
| --- | --- | --- | --- |
| P1 | Real editor capture coverage | Replace the placeholders with genuine Index screenshots and GIFs following the [Media Inventory](MEDIA_INVENTORY.md) | Not yet captured |
| P1 | Critical undo and hierarchy operations | Verify attached Actors, Folder moves, duplication and native transaction boundaries in UE 5.8 | Not run |
| P1 | Level saves under OFPA/Perforce | Test checkout, Save All, reload, Undo and multi-Level boundaries | Not run |
| P1 | Live website behavior | Test Pages deployment, full-text search, relative image paths, breadcrumbs and mobile widths | Not run |
| P1 | Source-level automated tests | Run `Automation RunTests Index` and record pass/fail output | Not run |
| P2 | Technical source link versioning | Pin source links to a tagged release/commit rather than changing main, once a versioned tag exists | Open |
| P2 | Accessibility | Test keyboard-only navigation, readable focus outlines, contrast, animated reduced-motion behavior | Not run |
| P2 | Performance under large Outliners | Verify drag/hover and sequence reconciliation with many loaded Actors/Folders | Not run |

## Content completeness standard

Every core workflow should answer: what the gesture does; what is eligible; how it differs from native parenting; where the resulting item lands; how saving and Undo work; what doesn't work; and how to diagnose a failed operation.

For technical readers, record the owning class/function, parent-group identity, transaction/metadata path, native event lifecycle, source limitations, and separation between source-reviewed logic and real executed tests.

## Already completed in this documentation pass

- Reviewed plugin descriptor, README, architecture notes, major module/column/state/interaction classes, settings and existing automation test definitions.
- Added consistent five-section information architecture and paired Design/Technical navigation.
- Reused actual Index screenshots/GIFs with the existing site's responsive media rules.
- Added source-directed media slots and detailed image/GIF capture instructions.
- Adapted the Focus documentation site's search, sidebar, breadcrumbs, Design/Technical switch, TOC and responsive presentation for Index.

## Limits of this audit

**Source review is not the same as proving runtime behavior.** The [Verification Matrix](VERIFICATION_MATRIX.md) lists checks that must be performed in an actual Unreal Editor project and browser before claiming the full documentation is verified.

---

[Documentation home](README.md) · [Roadmap](ROADMAP.md) · [Media Inventory](MEDIA_INVENTORY.md)
