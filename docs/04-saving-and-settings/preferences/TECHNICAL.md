# Project Settings and Visual Feedback — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Settings class and defaults

`UIndexSettings` derives from `UDeveloperSettings` with `config=Editor` and `defaultconfig`. Its category is Plugins. Defaults: ordering enabled, Above `FLinearColor(0,0.65,1,1)`, Below `FLinearColor(0,0.35,1,1)`, Edge Threshold `10.0f`, and Alt modifier.

The threshold property has `ClampMin=1` and `ClampMax=45`; `IndexInteraction::ResolveZone` also clamps the runtime normalized fraction to 0.01–0.45.

## Refresh behavior

`PostEditChangeProperty` calls `IndexInteraction::RefreshAllOutliners`. Input processing reads settings when resolving a drag and drawing its preview.

## Storage separation

Editor configuration values aren't the `Index.Hierarchy.V1` Level metadata. Changing an insertion color or modifier never changes the saved sibling sequence.

Source: [IndexSettings.h](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Public/IndexSettings.h) and [IndexSettings.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSettings.cpp).

---

[Read Design](DESIGN.md) · [Source](https://github.com/mippi-the-dork/Index/tree/main/Source/Index) · Execution testing not yet verified.
