# World Partition, Levels and Editor Scope — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Module declaration

`Index.uplugin` declares a `PostEngineInit` `Editor` module with `PlatformAllowList: ["Win64"]`, and no content. Registration occurs on `FSceneOutlinerModule::OnCreateActorBrowserColumns`, limited to Actor Browser contexts.

## Loaded item validation

The drag parser builds `FIndexSequenceItem` objects from valid `FActorTreeItem` or `FActorFolderTreeItem` rows. Unloaded Actors do not produce a valid loaded source/target row in this normal path.

## Per-Level state

`FIndexSequenceGroupKey` includes associated `ULevel`, root object and parent identity, preventing unrelated Level sibling sequences from being treated as one valid group.

## Lifecycle

Map changes invalidate state. `OnObjectsReplaced` handles Blueprint Actor reconstruction by clearing stale caches. Registered weak Outliner handles support refresh across windows.

## Non-goals

No runtime module, replacement Outliner mode or arbitrary Scene Outliner plugin integration is provided. Source compatibility with another engine minor release is distinct from binary compatibility.

---

[Read Design](DESIGN.md) · [Source](https://github.com/mippi-the-dork/Index/tree/main/Source/Index) · Execution testing not yet verified.
