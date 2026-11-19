# Release from Parent — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Explicit special actions

`IndexInteraction` distinguishes `ActorDetach`, `ActorMoveOutOfFolder`, `ActorDetachToFolder` and `FolderDetach` according to the native parent of the dragged item and the destination. Each action uses its corresponding native hierarchy path, not an invented ordering-only relationship.

## Ordering after release

Index reconciles the old and resulting parent tiers and puts the released root after its former parent where that relationship is permitted. It must preserve the moved root's attached descendants and nested Folder content.

## Input separation

Hovering a valid release target clears conflicting edge-insertion state; a mouse-up applies only the cached special action. A center drag that doesn't resolve to an eligible special target remains native.

## Transactions

Native hierarchy and Index metadata modifications can be distinct transaction changes. Test Undo/Redo with existing saved order to confirm both parent relationships and sibling positions restore properly.

---

For the workflow, [read Design](DESIGN.md). [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp).

**Validation:** Source review is not a passed Unreal Editor test.
