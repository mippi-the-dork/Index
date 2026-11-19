# Force Reorder Modifier — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Implementation

`IsModifierDown` in `IndexInteraction.cpp` checks `UIndexSettings::ReorderModifierKey` against the pointer event's current Alt, Shift or Control key state. When `EIndexModifierKey::None` is selected, it returns false.

`ResolveZone` tests the modifier *before* the edge percentage. With the key active, `Fraction < 0.5` means Before and all remaining half-row positions mean After. Without the key, only the configured edge bands qualify.

## Settings source

`UIndexSettings` is a `UDeveloperSettings` class with `config=Editor` and `defaultconfig`. The default enum value is Alt; `PostEditChangeProperty` calls `IndexInteraction::RefreshAllOutliners()` so changes become visible in current Outliners.

## Scope and distinction

Force Reorder does not change saved order by itself. It changes **hover classification** during a drag; a valid insertion must still pass sequence validation and be committed on release. It does not introduce a new hierarchy structure or bypass invalid level relationships.

## Modifier conflicts

Because modifier input is taken from the normal pointer event, other editor workflows may have their own key semantics. Test the selected modifier with your project-specific editor tools.

---

[User-facing Design guide](DESIGN.md) · [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp)

**Validation:** Source inspected; live-editor tests remain separate.
