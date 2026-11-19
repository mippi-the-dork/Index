# Maintaining Index Documentation

The site is served from Index's `/docs` directory and reuses the presentation language of the Mippi portfolio/Focus documentation site.

## Design and Technical ownership

Every topic has two complementary guides. **DESIGN.md** is for users: gestures, controls, expected results, use cases, screenshots and troubleshooting. **TECHNICAL.md** is for implementers: owning C++ classes, Unreal editor callbacks, sequence identity, transaction and metadata behavior, and integration boundaries.

Don't duplicate the same README section across both pages. Check source before describing a feature as shipped; don't invent tests or new release milestones.

## Update workflow

1. Review the changed source and the appropriate existing topic guides.
2. Update Design and Technical together, and adjust its category table when introducing/removing a topic.
3. Update the root category count table and verify all local Markdown links.
4. Update [Media Inventory](MEDIA_INVENTORY.md) and the JSON registry when adding, replacing or removing media. New placements need a reader-facing caption **plus a second line of precise capture instructions**, labeled Image or GIF.
5. Record actual manual and automated test results in [Verification Matrix](VERIFICATION_MATRIX.md); source review alone doesn't qualify as Passed.
6. Review full-text search, navigation, CSS media bounds, browser widths, and screen-reader/keyboard access.

## Media replacements

The neutral `assets/media-placeholder.png` is an authentic supplied placeholder, not a rendered Index screenshot. Each current placeholder declares the final image/GIF type and a specific capture goal. When replacing, add the real file under `docs/assets/media/`, update the figure's source and alt text if needed, and preserve an accurate public caption. Existing authentic Index screenshots/GIFs should be reused where relevant.

## Versions and roadmap

The site currently documents Index **v1.0.0**. Its [Roadmap](ROADMAP.md) distinguishes the released version from future questions, not an invented v2.0 commitment. Pin technical links to version tags when available.

## GitHub Pages

Configure repository **Settings → Pages → Deploy from a branch → main → /docs** after merge. Expected website: https://mippi-the-dork.github.io/Index/ . GitHub Pages activation and live deployment are separate from merging documentation.
