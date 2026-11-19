# Supported Actions and Limitations — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Supported targets

`IndexInteraction::BuildSequencePayload` recognizes the relevant native `FActorTreeItem` and `FActorFolderTreeItem` entries. `FIndexSequenceState::CanInsertItemsRelative` validates compatible source/target group rules.

## Column scope

The hidden `FIndexColumn` is registered through `OnCreateActorBrowserColumns` and is not a general-purpose sorting interface for Blueprint component editors, runtime Actor trees or arbitrary custom Outliners.

## Per-Level persistence

The state is written on `ULevel` metadata; cross-level custom ordering isn't represented as one shared sequence. World Partition unloaded objects may still have persistent identities but cannot be dragged via normal loaded Outliner items.

## Unsupported effects

No runtime dependency, Actor component, special asset or 3D transform setter is authored by this plugin. The code avoids custom row backgrounds and alternative Outliner modes.

## Other plugins

Index consumes/observes native Slate events during eligible drag operations. Interactions with third-party Outliner modifications may require project-specific testing; the presence of a World Outliner-like widget is not sufficient to guarantee compatibility.

---

[Read Design](DESIGN.md) · [Index source](https://github.com/mippi-the-dork/Index/tree/main/Source/Index) · Unreal Editor execution has not been verified in this pass.
