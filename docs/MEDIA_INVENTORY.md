# Index Documentation Media Inventory

This inventory tracks the **23 planned screenshots, diagrams and demonstrations** throughout the Index manual. All planned slots currently display the same neutral placeholder provided for the Focus documentation process.

Each figure has **two separate captions**: a natural reader-facing explanation and a clearly labeled **Capture instructions · Image/GIF** line specifying the exact interface, states or sequence to capture. The instructions also live in [the JSON media registry](assets/media-slots.json) under `capture_instructions`.

## Real Index imagery already in use

The current README/website reuses genuine repository media, which should **not** be replaced by placeholders:

- [Index-Hero.png](https://github.com/mippi-the-dork/Index/blob/main/Doc/Images/Index-Hero.png) — website hero and header branding context.
- [Index-Reordering.gif](https://github.com/mippi-the-dork/Index/blob/main/Doc/Images/Index-Reordering.gif) — Before/After reordering.
- [Index-Force-Reorder.gif](https://github.com/mippi-the-dork/Index/blob/main/Doc/Images/Index-Force-Reorder.gif) — modifier-assisted large insertion targets.
- [Index-Insert-Above.png](https://github.com/mippi-the-dork/Index/blob/main/Doc/Images/Index-Insert-Above.png) and [Index-Insert-Below.png](https://github.com/mippi-the-dork/Index/blob/main/Doc/Images/Index-Insert-Below.png) — insertion indicators.
- [Index-Settings.png](https://github.com/mippi-the-dork/Index/blob/main/Doc/Images/Index-Settings.png) — actual Project Settings.

## Placeholder captures

| Documentation page | Final type | Intended file | Slot ID |
| --- | --- | --- | --- |
| [01-getting-started/overview/DESIGN.md](01-getting-started/overview/DESIGN.md) | IMAGE | `assets/media/overview-ordering.png` | overview-ordering |
| [01-getting-started/installation/DESIGN.md](01-getting-started/installation/DESIGN.md) | IMAGE | `assets/media/installation-folder.png` | installation-folder |
| [01-getting-started/quickstart/DESIGN.md](01-getting-started/quickstart/DESIGN.md) | GIF | `assets/media/quickstart-force.gif` | quickstart-force |
| [02-ordering-and-dragging/edge-zones/DESIGN.md](02-ordering-and-dragging/edge-zones/DESIGN.md) | IMAGE | `assets/media/edge-zone-comparison.png` | edge-zone-comparison |
| [02-ordering-and-dragging/force-reorder/DESIGN.md](02-ordering-and-dragging/force-reorder/DESIGN.md) | GIF | `assets/media/force-modifier-gestures.gif` | force-modifier-gestures |
| [02-ordering-and-dragging/drag-feedback/DESIGN.md](02-ordering-and-dragging/drag-feedback/DESIGN.md) | IMAGE | `assets/media/drag-feedback-examples.png` | drag-feedback-examples |
| [02-ordering-and-dragging/mixed-siblings/DESIGN.md](02-ordering-and-dragging/mixed-siblings/DESIGN.md) | IMAGE | `assets/media/mixed-sibling-order.png` | mixed-sibling-order |
| [02-ordering-and-dragging/multi-selection/DESIGN.md](02-ordering-and-dragging/multi-selection/DESIGN.md) | GIF | `assets/media/multi-block-drag.gif` | multi-block-drag |
| [03-hierarchy-and-lifecycle/parenting/DESIGN.md](03-hierarchy-and-lifecycle/parenting/DESIGN.md) | GIF | `assets/media/parent-vs-insert.gif` | parent-vs-insert |
| [03-hierarchy-and-lifecycle/releasing-parents/DESIGN.md](03-hierarchy-and-lifecycle/releasing-parents/DESIGN.md) | GIF | `assets/media/release-from-parent.gif` | release-from-parent |
| [03-hierarchy-and-lifecycle/new-items/DESIGN.md](03-hierarchy-and-lifecycle/new-items/DESIGN.md) | GIF | `assets/media/new-items-top.gif` | new-items-top |
| [03-hierarchy-and-lifecycle/duplication/DESIGN.md](03-hierarchy-and-lifecycle/duplication/DESIGN.md) | GIF | `assets/media/duplicate-beside-source.gif` | duplicate-beside-source |
| [03-hierarchy-and-lifecycle/rename-filter-sort/DESIGN.md](03-hierarchy-and-lifecycle/rename-filter-sort/DESIGN.md) | GIF | `assets/media/rename-filter-sequence.gif` | rename-filter-sequence |
| [03-hierarchy-and-lifecycle/blank-space/DESIGN.md](03-hierarchy-and-lifecycle/blank-space/DESIGN.md) | GIF | `assets/media/blank-space-root.gif` | blank-space-root |
| [03-hierarchy-and-lifecycle/deletion/DESIGN.md](03-hierarchy-and-lifecycle/deletion/DESIGN.md) | IMAGE | `assets/media/deletion-sequence.png` | deletion-sequence |
| [04-saving-and-settings/persistence/DESIGN.md](04-saving-and-settings/persistence/DESIGN.md) | GIF | `assets/media/persist-reopen.gif` | persist-reopen |
| [04-saving-and-settings/undo/DESIGN.md](04-saving-and-settings/undo/DESIGN.md) | GIF | `assets/media/undo-reorder.gif` | undo-reorder |
| [04-saving-and-settings/preferences/DESIGN.md](04-saving-and-settings/preferences/DESIGN.md) | IMAGE | `assets/media/settings-edge-zones.png` | settings-edge-zones |
| [04-saving-and-settings/compatibility/DESIGN.md](04-saving-and-settings/compatibility/DESIGN.md) | IMAGE | `assets/media/wp-loaded-rows.png` | wp-loaded-rows |
| [05-architecture-and-support/architecture/DESIGN.md](05-architecture-and-support/architecture/DESIGN.md) | IMAGE | `assets/media/architecture-overview.png` | architecture-overview |
| [05-architecture-and-support/identity-and-reconciliation/DESIGN.md](05-architecture-and-support/identity-and-reconciliation/DESIGN.md) | IMAGE | `assets/media/identity-parent-tiers.png` | identity-parent-tiers |
| [05-architecture-and-support/limitations/DESIGN.md](05-architecture-and-support/limitations/DESIGN.md) | IMAGE | `assets/media/limits-valid-drops.png` | limits-valid-drops |
| [05-architecture-and-support/troubleshooting/DESIGN.md](05-architecture-and-support/troubleshooting/DESIGN.md) | IMAGE | `assets/media/troubleshooting-edge.png` | troubleshooting-edge |

## Replacing a placeholder

1. Capture the actual Index UI or create an accurate technical diagram described in the matching page's **Capture instructions**.
2. Save the image or GIF as the intended filename under `docs/assets/media/` (create that folder when the first final asset is supplied).
3. Replace the figure's `src` with the correct relative asset path. Maintain natural aspect ratio and article-width sizing.
4. Keep the original reader-facing caption if it still accurately explains the final content. Adjust the alt description when the framing differs.
5. Update the JSON registry and this inventory to mark the final capture complete; check it in GitHub Markdown and the live Pages site.

The presence of a slot in this inventory **does not mean the final screenshot or animation has been captured**. It is deliberately a production checklist and a reader-facing placeholder workflow.

[Documentation home](README.md) · [Verification Matrix](VERIFICATION_MATRIX.md)
