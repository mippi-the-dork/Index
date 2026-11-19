# Multi-Selection Reordering — Technical

**Index v1.0.0 · Source-reviewed implementation**

## Canonicalize selected roots

`FIndexSequenceState::CollapseToTopLevelRoots` reduces a selection to top-level moving roots, removing descendants already carried by a selected ancestor. `SortItemsForMove` uses their existing visual hierarchy paths to keep original order.

`IndexInteraction::BuildSequencePayload` parses the native drag operation into supported `FIndexSequenceItem` entries. `CanInsertItemsRelative` validates target compatibility before the operation is displayed/committed.

## Insertion semantics

`InsertItemsRelativeImpl` removes the selected roots from their existing sibling sequences, finds the target sibling placement, and inserts the block in the preserved source order. It rejects unsupported cycles and invalid relationships.

## Incremental native callbacks

When Unreal performs a multi-item move one Actor at a time, `FIndexSequenceState` retains a transient native-move baseline, departure-tier sequences and historical visual paths. This prevents intermediate Outliner refreshes from reversing the moved block.

## Transaction boundary

An explicit Index reorder creates an `FScopedTransaction` and stores hierarchy metadata deltas via command changes; broader native hierarchy moves may be reconciled across Unreal's own transactions.

## Verification

The repository's `Index.HierarchyState` automation spec includes a move-before test with reverse-supplied input that asserts the resulting order remains canonical. This demonstrates one source-level test definition; it is **not** evidence that the suite was executed in this session.

---

[User-facing Design guide](DESIGN.md) · [IndexInteraction.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexInteraction.cpp) · [IndexSequenceState.cpp](https://github.com/mippi-the-dork/Index/blob/main/Source/Index/Private/IndexSequenceState.cpp)

**Validation:** Source inspected; live-editor tests remain separate.
