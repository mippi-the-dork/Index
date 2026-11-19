# Index v1.0.0 Verification Matrix

**Status:** Checklist authored; no Index Unreal automation, hands-on editor or live browser test has been executed in this documentation pass.

Record for each actual execution: Unreal exact version, Index commit/release, World Partition/OFPA mode, source control, date, steps, expected/observed result, test output and attachments. Use **Passed**, **Failed**, **Blocked**, or **Not applicable** only with evidence.

## Unreal Editor and ordering

| ID | Scenario | Expected result | Status |
| --- | --- | --- | --- |
| IDX-01 | Enable Index and open standard World Outliner | Manual ordering available, native rows unchanged | Not run |
| IDX-02 | Drag to top and bottom row edges | Before/After full-width line, intended sibling position | Not run |
| IDX-03 | Drag to center of eligible Actor/Folder | Native hierarchy tooltip and appropriate parent/move action | Not run |
| IDX-04 | Hold Alt, Shift, Ctrl and disabled setting | Configured half-row reorder zones; disabled keeps edge zones | Not run |
| IDX-05 | Change Edge Threshold from 1–45% | Hit regions track configured fraction without breaking native center | Not run |
| IDX-06 | Change Above/Below line colors | Correct feedback with no row-height/selection visual regressions | Not run |
| IDX-07 | Mix Actor and Folder siblings, move child tier independently | Exact combined order preserved per parent | Not run |
| IDX-08 | Move nonadjacent multi-selection | One contiguous block in previous visual order | Not run |
| IDX-09 | Select parent and descendant together | Descendant carried once; hierarchy unflattened | Not run |
| IDX-10 | Drop child on its own Actor/Folder parent | Detach/outdent and placement after former parent | Not run |
| IDX-11 | Drop selection into Outliner blank root area | Block moves to compatible root bottom | Not run |
| IDX-12 | Move item at boundary of expanded parent and first child | Correct child tier insertion, no accidental outer-level placement | Not run |
| IDX-13 | Create Actor and Folder | New item starts at top of local tier, existing order remains | Not run |
| IDX-14 | Duplicate Actor and nested Folder | Copies adjacent to source if unambiguous; hierarchy retained | Not run |
| IDX-15 | Ambiguous multi-tier duplicate | Documented top-of-tier fallback | Not run |
| IDX-16 | Rename Actor/Folder and filter the Outliner | Saved order unchanged | Not run |
| IDX-17 | Temporarily sort Outliner by another column | Manual metadata remains; Index reorder restores manual sort presentation | Not run |
| IDX-18 | Delete middle Actor/Folder | Remaining siblings keep order | Not run |
| IDX-19 | Undo/Redo reorder and native Folder move | Prior parent and sequence restored; multi-step cases documented | Not run |
| IDX-20 | Save Level; close/reopen Unreal | Saved manual order restored from Level package metadata | Not run |
| IDX-21 | Reorder under Perforce with OFPA | Checkout/save covers necessary Level package; no unsupported guarantees | Not run |
| IDX-22 | Try unloaded WP Actor and cross-Level target | Unsupported operation unavailable/rejected safely | Not run |
| IDX-23 | Open multiple standard World Outliner windows | Correct refresh; saved ordering remains coherent | Not run |
| IDX-24 | Large hierarchy with 1,000+ loaded rows | No unacceptable drag/update hitches or stale overlay | Not run |
| IDX-25 | Disable/re-enable plugin during editor session | Input/delegates/overlay cleaned up safely | Not run |

## Native Unreal Automation Framework

| ID | Action | Expected | Status |
| --- | --- | --- | --- |
| AUTO-01 | Run `Automation RunTests Index` in UE 5.8 Editor | Existing Index.HierarchyState spec reports actual results | Not run |
| AUTO-02 | Review automation log for all Index.HierarchyState cases | No invented passes; failures linked to exact tests | Not run |

## GitHub Pages

| ID | Test | Expected | Status |
| --- | --- | --- | --- |
| WEB-01 | Open site desktop and mobile | No horizontal overflow; image placeholders fit article | Not run |
| WEB-02 | Browse all five sections and Design/Technical pairs | Correct navigation, breadcrumb, TOC and back/forward behavior | Not run |
| WEB-03 | Search exact title and term only in page body | Immediate title results and full-text excerpts | Not run |
| WEB-04 | Check hero, plugin header icon and real GIFs | Correct branding and assets, no broken image | Not run |
| WEB-05 | Test keyboard focus, search keys, screen reader and contrast | Navigation and controls usable | Not run |
| WEB-06 | Check each placeholder capture instruction | Second line readable and matching its intended Image/GIF type | Not run |

**Important:** Passing Markdown link checks, JS parsing and source inspection does **not** mark these runtime/browser tests Passed.

[Audit](DOCUMENTATION_AUDIT.md) · [Media inventory](MEDIA_INVENTORY.md) · [Index source](https://github.com/mippi-the-dork/Index)
