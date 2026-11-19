# Installation and Compatibility — Technical

**Index v1.0.0 · Implementation reference**

## Plugin descriptor

`Index.uplugin` declares `Version: 100`, `VersionName: "1.0.0"`, `EngineVersion: "5.8.0"`, `Type: "Editor"`, `LoadingPhase: "PostEngineInit"`, `PlatformAllowList: ["Win64"]` and `CanContainContent: false`. EngineVersion denotes the descriptor target, not proof of all patch-version binary compatibility.

## Startup

`FIndexModule` loads `SceneOutliner`, registers a column factory for standard Actor Browser instances, installs an `IInputProcessor` if Slate is initialized, and subscribes to editor callbacks for Undo/Redo, map changes, Actor placement, duplication, Folder changes and Actor attachment/deletion.

Shutdown removes those delegates, unregisters the input preprocessor, cancels pending duplicate finalization, and invalidates the sequence cache.

## Build and distribution

`Source/Index/Index.Build.cs` defines the editor-side module dependencies. A downloaded source checkout normally requires compilation for its exact engine/toolchain; a precompiled archive must match its build target.

## Packaging boundary

Because the sole module is Editor-only and `CanContainContent` is false, Index itself adds no runtime module or content assets. Native Actor/Folder data still belongs to Unreal. The feature is intentionally absent from packaged games.

References: [Index.uplugin](https://github.com/mippi-the-dork/Index/blob/main/Index.uplugin), [Index.Build.cs](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Index.Build.cs), and [IndexModule.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexModule.cpp).

---

For expected user behavior, [read Design](DESIGN.md). Source: [Index repository](https://github.com/mippi-the-dork/Index/tree/main/Source/Index).

**Verification:** Reviewed against published source; in-editor QA is tracked separately.
