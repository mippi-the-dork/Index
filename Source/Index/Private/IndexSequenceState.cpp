#include "IndexSequenceState.h"

#include "Editor.h"
#include "Algo/Reverse.h"
#include "EditorActorFolders.h"
#include "Engine/Level.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"
#include "Misc/Change.h"
#include "Misc/ITransaction.h"
#include "CoreGlobals.h"
#include "ScopedTransaction.h"
#include "UObject/MetaData.h"
#include "UObject/Package.h"

#define LOCTEXT_NAMESPACE "IndexSequenceState"

namespace IndexSequenceStatePrivate
{
    static const FName HierarchyMetaDataKey(TEXT("Index.Hierarchy.V1"));
    static int32 ExplicitReorderDepth = 0;

    struct FCachedLevelState
    {
        FString SourceValue;
        TMap<FString, TArray<FString>> ChildrenByParent;
        bool bLoaded = false;
        bool bPersisted = false;
    };

    static TMap<TWeakObjectPtr<ULevel>, FCachedLevelState> Cache;

    // Native hierarchy operations such as Create Folder containing selection can move
    // several rows one at a time while the Scene Outliner re-sorts between individual
    // moves. Keep a short-lived immutable snapshot of the pre-move hierarchy so every
    // row in that native batch is compared against the same origin order.
    struct FNativeMoveBaseline
    {
        uint64 CapturedFrame = 0;
        bool bValid = false;
        TMap<FString, TArray<int32>> VisualPathById;
        TMap<FString, FString> ParentById;

        // Short-lived native move baselines deliberately expire after a few frames,
        // but Undo may happen much later. Retain the most recently observed visual
        // path for each item within every parent it has occupied during this editor
        // session. Post-Undo/Redo reconciliation can then restore a returning row to
        // its historical local slot without changing ordinary native move semantics.
        TMap<FString, TMap<FString, TArray<int32>>> HistoricalVisualPathByIdAndParent;

        struct FDepartureSiblingSequence
        {
            FString Parent;
            TArray<FString> Sequence;
            uint64 Serial = 0;
        };

        // Exact source-tier snapshot captured when a tracked row is first reconciled
        // into a different parent. Unlike per-item historical indices, this preserves
        // the entire sibling sequence before the source tier is compressed by later
        // moves. Post-Undo reconciliation can therefore restore [A,B] or [G,H]
        // atomically once the complete membership returns.
        TMap<FString, FDepartureSiblingSequence> DepartureSequenceById;

        // A single user command such as Create Folder with Selection can be recorded
        // by Unreal as several adjacent native transactions (for example, one move
        // per selected folder/actor). Every one of those transactions must restore
        // the same Index ordering state that existed before the overall command
        // began. Keep that pre-command serialized value alive across the tiny native
        // transaction burst instead of recapturing partially reconciled metadata.
        uint64 UndoAnchorFrame = 0;
        bool bUndoAnchorValid = false;
        bool bUndoAnchorHadValue = false;
        FString UndoAnchorValue;
    };

    static TMap<TWeakObjectPtr<ULevel>, FNativeMoveBaseline> NativeMoveBaselines;
    static uint64 DepartureSequenceSerial = 0;
    static uint64 PostUndoRedoRestoreUntilFrame = 0;

    // Native hierarchy transactions already carry Index's exact serialized hierarchy
    // baseline through FNativeHierarchyUndoBaselineChange. During Undo, retain every
    // baseline restored by that transaction for a few frames so the following
    // Outliner sort can use the transaction's own sibling sequence instead of
    // reconstructing it from compressed session history.
    struct FPendingUndoRestoreState
    {
        uint64 RecordedFrame = 0;
        TArray<TMap<FString, TArray<FString>>> CandidateSnapshots;
    };

    static TMap<TWeakObjectPtr<ULevel>, FPendingUndoRestoreState> PendingUndoRestoreStates;

    class FScopedExplicitReorderGuard
    {
    public:
        FScopedExplicitReorderGuard() { ++ExplicitReorderDepth; }
        ~FScopedExplicitReorderGuard() { ExplicitReorderDepth = FMath::Max(0, ExplicitReorderDepth - 1); }
    };

    static ULevel* GetLevel(const FIndexSequenceItem& Item)
    {
        if (!Item.IsValid())
        {
            return nullptr;
        }
        return Item.IsActor() ? Item.Actor->GetLevel() : Item.Folder.GetRootObjectAssociatedLevel();
    }

    static FString ActorId(const AActor* Actor)
    {
        if (!IsValid(Actor))
        {
            return FString();
        }
        const FGuid& Guid = Actor->GetActorGuid();
        return Guid.IsValid()
            ? FString::Printf(TEXT("A:%s"), *Guid.ToString(EGuidFormats::Digits))
            : FString::Printf(TEXT("AO:%s"), *Actor->GetPathName());
    }

    static FString RootId(const FFolder::FRootObject& RootObject, const ULevel* Level)
    {
        UObject* Root = FFolder::GetRootObjectPtr(RootObject);
        if (!Root || Root == Level)
        {
            return TEXT("R:LEVEL");
        }
        if (const AActor* RootActor = Cast<AActor>(Root))
        {
            return FString::Printf(TEXT("R:%s"), *ActorId(RootActor));
        }
        return FString::Printf(TEXT("R:O:%s"), *Root->GetPathName());
    }

    static FString FolderId(const FFolder& Folder)
    {
        if (!Folder.IsValid())
        {
            return FString();
        }
        const FGuid& Guid = Folder.GetActorFolderGuid();
        if (Guid.IsValid())
        {
            return FString::Printf(TEXT("F:%s"), *Guid.ToString(EGuidFormats::Digits));
        }
        ULevel* Level = Folder.GetRootObjectAssociatedLevel();
        return FString::Printf(
            TEXT("FP:%s:%s"),
            *RootId(Folder.GetRootObject(), Level),
            *Folder.GetPath().ToString());
    }

    static FString ItemId(const FIndexSequenceItem& Item)
    {
        if (!Item.IsValid())
        {
            return FString();
        }
        return Item.IsActor() ? ActorId(Item.Actor.Get()) : FolderId(Item.Folder);
    }

    static FString ParentId(const FIndexSequenceItem& Item)
    {
        if (!Item.IsValid())
        {
            return FString();
        }

        ULevel* Level = GetLevel(Item);
        if (Item.IsActor())
        {
            AActor* Actor = Item.Actor.Get();
            if (AActor* ParentActor = Actor->GetAttachParentActor())
            {
                return ActorId(ParentActor);
            }

            const FFolder Folder = Actor->GetFolder();
            if (Folder.IsValid() && !Folder.IsNone())
            {
                return FolderId(Folder);
            }
            return RootId(Actor->GetFolderRootObject(), Level);
        }

        const FFolder Parent = Item.Folder.GetParent();
        if (Parent.IsValid() && !Parent.IsNone())
        {
            return FolderId(Parent);
        }
        return RootId(Item.Folder.GetRootObject(), Level);
    }

    static TArray<int32> StoredVisualPath(const FCachedLevelState& State, const FString& ItemIdValue)
    {
        TArray<int32> ReversePath;
        FString CurrentId = ItemIdValue;

        // Build a visual path entirely from Index's persisted hierarchy snapshot.
        // This is intentionally independent of the actor/folder's current native
        // parent so it still represents where a row lived immediately before a
        // native hierarchy operation moved it.
        int32 Guard = 0;
        while (!CurrentId.IsEmpty() && Guard++ < 64)
        {
            bool bFoundParent = false;
            for (const TPair<FString, TArray<FString>>& Pair : State.ChildrenByParent)
            {
                const int32 ChildIndex = Pair.Value.IndexOfByKey(CurrentId);
                if (ChildIndex != INDEX_NONE)
                {
                    ReversePath.Add(ChildIndex);
                    CurrentId = Pair.Key;
                    bFoundParent = true;
                    break;
                }
            }

            if (!bFoundParent)
            {
                break;
            }
        }

        Algo::Reverse(ReversePath);
        return ReversePath;
    }

    static bool StoredPathLess(const TArray<int32>& A, const TArray<int32>& B)
    {
        const int32 Count = FMath::Min(A.Num(), B.Num());
        for (int32 Index = 0; Index < Count; ++Index)
        {
            if (A[Index] != B[Index])
            {
                return A[Index] < B[Index];
            }
        }
        return A.Num() < B.Num();
    }

    static FNativeMoveBaseline& GetNativeMoveBaseline(ULevel* Level, const FCachedLevelState& State)
    {
        FNativeMoveBaseline& Baseline = NativeMoveBaselines.FindOrAdd(TWeakObjectPtr<ULevel>(Level));

        // A native multi-row hierarchy change is synchronous in normal editor use, but
        // the Outliner may defer one of its refreshes to the following frame. Keep the
        // same baseline for a tiny two-frame window so an interleaved refresh cannot
        // turn an ordered block into repeated insert-at-top operations.
        const bool bExpired = !Baseline.bValid || GFrameCounter > Baseline.CapturedFrame + 2;
        if (bExpired)
        {
            Baseline.CapturedFrame = GFrameCounter;
            Baseline.bValid = true;
            Baseline.VisualPathById.Reset();
            Baseline.ParentById.Reset();

            for (const TPair<FString, TArray<FString>>& Pair : State.ChildrenByParent)
            {
                for (const FString& ChildId : Pair.Value)
                {
                    if (ChildId.IsEmpty())
                    {
                        continue;
                    }
                    const TArray<int32> Path = StoredVisualPath(State, ChildId);
                    Baseline.ParentById.Add(ChildId, Pair.Key);
                    Baseline.VisualPathById.Add(ChildId, Path);
                    Baseline.HistoricalVisualPathByIdAndParent
                        .FindOrAdd(ChildId)
                        .Add(Pair.Key, Path);
                }
            }
        }

        return Baseline;
    }

    static bool HasExactMembership(const TArray<FString>& A, const TArray<FString>& B)
    {
        if (A.Num() != B.Num())
        {
            return false;
        }
        for (const FString& Id : A)
        {
            if (!B.Contains(Id))
            {
                return false;
            }
        }
        return true;
    }

    static void CaptureDepartureSequenceFromState(
        ULevel* Level,
        const FCachedLevelState& State,
        const FString& Id)
    {
        if (!Level || Id.IsEmpty())
        {
            return;
        }

        for (const TPair<FString, TArray<FString>>& Pair : State.ChildrenByParent)
        {
            if (!Pair.Value.Contains(Id))
            {
                continue;
            }

            FNativeMoveBaseline& Baseline =
                NativeMoveBaselines.FindOrAdd(TWeakObjectPtr<ULevel>(Level));
            FNativeMoveBaseline::FDepartureSiblingSequence& Record =
                Baseline.DepartureSequenceById.FindOrAdd(Id);
            Record.Parent = Pair.Key;
            Record.Sequence = Pair.Value;
            Record.Serial = ++DepartureSequenceSerial;
            return;
        }
    }

    static bool TryGetDepartureRestoreSequence(
        ULevel* Level,
        const FString& Parent,
        const TArray<FString>& CurrentIds,
        TArray<FString>& OutSequence)
    {
        OutSequence.Reset();
        if (!Level || Parent.IsEmpty() || CurrentIds.IsEmpty())
        {
            return false;
        }

        const FNativeMoveBaseline* Baseline =
            NativeMoveBaselines.Find(TWeakObjectPtr<ULevel>(Level));
        if (!Baseline)
        {
            return false;
        }

        uint64 BestSerial = 0;
        for (const FString& Id : CurrentIds)
        {
            const FNativeMoveBaseline::FDepartureSiblingSequence* Record =
                Baseline->DepartureSequenceById.Find(Id);
            if (!Record
                || Record->Parent != Parent
                || !HasExactMembership(Record->Sequence, CurrentIds))
            {
                continue;
            }

            if (Record->Serial >= BestSerial)
            {
                BestSerial = Record->Serial;
                OutSequence = Record->Sequence;
            }
        }

        return BestSerial != 0 && !OutSequence.IsEmpty();
    }

    static bool IsPostUndoRedoRestoreWindow()
    {
        return PostUndoRedoRestoreUntilFrame != 0
            && GFrameCounter <= PostUndoRedoRestoreUntilFrame;
    }

    static void BeginPostUndoRedoRestoreWindow()
    {
        // PostUndoRedo is an explicit command boundary, unlike the speculative
        // forward-move batching windows. A small allowance simply covers the native
        // Outliner refresh/sort that may land on one of the next few frames.
        PostUndoRedoRestoreUntilFrame = GFrameCounter + 8;
    }

    static void RemapNativeMoveBaselineId(ULevel* Level, const FString& OldId, const FString& NewId)
    {
        if (!Level || OldId.IsEmpty() || NewId.IsEmpty() || OldId == NewId)
        {
            return;
        }

        if (FNativeMoveBaseline* Baseline = NativeMoveBaselines.Find(TWeakObjectPtr<ULevel>(Level)))
        {
            if (TArray<int32>* Path = Baseline->VisualPathById.Find(OldId))
            {
                Baseline->VisualPathById.Add(NewId, *Path);
                Baseline->VisualPathById.Remove(OldId);
            }
            if (FString* Parent = Baseline->ParentById.Find(OldId))
            {
                Baseline->ParentById.Add(NewId, *Parent);
                Baseline->ParentById.Remove(OldId);
            }
            if (TMap<FString, TArray<int32>>* History = Baseline->HistoricalVisualPathByIdAndParent.Find(OldId))
            {
                Baseline->HistoricalVisualPathByIdAndParent.Add(NewId, *History);
                Baseline->HistoricalVisualPathByIdAndParent.Remove(OldId);
            }

            if (FNativeMoveBaseline::FDepartureSiblingSequence* Departure = Baseline->DepartureSequenceById.Find(OldId))
            {
                Baseline->DepartureSequenceById.Add(NewId, *Departure);
                Baseline->DepartureSequenceById.Remove(OldId);
            }

            for (TPair<FString, FNativeMoveBaseline::FDepartureSiblingSequence>& Pair : Baseline->DepartureSequenceById)
            {
                if (Pair.Value.Parent == OldId)
                {
                    Pair.Value.Parent = NewId;
                }
                for (FString& SequenceId : Pair.Value.Sequence)
                {
                    if (SequenceId == OldId)
                    {
                        SequenceId = NewId;
                    }
                }
            }
        }
    }

    static FString Serialize(const TMap<FString, TArray<FString>>& Map)
    {
        TArray<FString> Parents;
        Map.GetKeys(Parents);
        Parents.Sort();

        TArray<FString> Lines;
        Lines.Reserve(Parents.Num() + 1);
        Lines.Add(TEXT("V1"));
        for (const FString& Parent : Parents)
        {
            const TArray<FString>* Children = Map.Find(Parent);
            if (!Children)
            {
                continue;
            }
            Lines.Add(Parent + TEXT("\t") + FString::Join(*Children, TEXT(",")));
        }
        return FString::Join(Lines, TEXT("\n"));
    }

    static void Deserialize(const FString& Value, TMap<FString, TArray<FString>>& OutMap)
    {
        OutMap.Reset();
        if (Value.IsEmpty())
        {
            return;
        }

        TArray<FString> Lines;
        Value.ParseIntoArrayLines(Lines, false);
        for (const FString& Line : Lines)
        {
            if (Line == TEXT("V1"))
            {
                continue;
            }
            FString Parent;
            FString ChildrenText;
            if (!Line.Split(TEXT("\t"), &Parent, &ChildrenText) || Parent.IsEmpty())
            {
                continue;
            }
            TArray<FString>& Children = OutMap.FindOrAdd(Parent);
            ChildrenText.ParseIntoArray(Children, TEXT(","), true);
        }
    }

    static void RecordUndoRestoreSnapshot(ULevel* Level, const FString& Value)
    {
        if (!Level)
        {
            return;
        }

        FPendingUndoRestoreState& RestoreState =
            PendingUndoRestoreStates.FindOrAdd(TWeakObjectPtr<ULevel>(Level));

        // All command changes in one Unreal transaction are replayed synchronously.
        // A later Undo naturally lands on another frame, so start a fresh candidate
        // collection instead of letting an older transaction influence this one.
        if (RestoreState.RecordedFrame != GFrameCounter)
        {
            RestoreState.RecordedFrame = GFrameCounter;
            RestoreState.CandidateSnapshots.Reset();
        }

        TMap<FString, TArray<FString>> Snapshot;
        Deserialize(Value, Snapshot);
        RestoreState.CandidateSnapshots.Add(MoveTemp(Snapshot));
    }

    static void ClearUndoRestoreSnapshots(ULevel* Level)
    {
        if (Level)
        {
            PendingUndoRestoreStates.Remove(TWeakObjectPtr<ULevel>(Level));
        }
    }

    static bool TryGetTransactionUndoSequence(
        ULevel* Level,
        const FString& Parent,
        const TArray<FString>& CurrentIds,
        TArray<FString>& OutSequence)
    {
        OutSequence.Reset();
        if (!Level || Parent.IsEmpty())
        {
            return false;
        }

        const FPendingUndoRestoreState* RestoreState =
            PendingUndoRestoreStates.Find(TWeakObjectPtr<ULevel>(Level));
        if (!RestoreState
            || RestoreState->CandidateSnapshots.IsEmpty()
            || GFrameCounter > RestoreState->RecordedFrame + 2)
        {
            return false;
        }

        // A transaction may contain more than one native baseline change. Search the
        // most recently replayed candidates first and accept only an exact sibling-set
        // match. This prevents a partial state such as [H] from overriding the older
        // transaction snapshot [G,H] once G has actually returned to Folder 4.
        for (int32 SnapshotIndex = RestoreState->CandidateSnapshots.Num() - 1; SnapshotIndex >= 0; --SnapshotIndex)
        {
            const TArray<FString>* Candidate =
                RestoreState->CandidateSnapshots[SnapshotIndex].Find(Parent);
            if (!Candidate || Candidate->Num() != CurrentIds.Num())
            {
                continue;
            }

            bool bExactMembership = true;
            for (const FString& Id : CurrentIds)
            {
                if (!Candidate->Contains(Id))
                {
                    bExactMembership = false;
                    break;
                }
            }

            if (bExactMembership)
            {
                OutSequence = *Candidate;
                return true;
            }
        }

        return false;
    }

    static FString ReadPersistedValue(ULevel* Level, bool& bOutHasValue)
    {
        bOutHasValue = false;
        if (!Level || !Level->GetPackage())
        {
            return FString();
        }
        FMetaData& Meta = Level->GetPackage()->GetMetaData();
        bOutHasValue = Meta.HasValue(Level, HierarchyMetaDataKey);
        return bOutHasValue ? Meta.GetValue(Level, HierarchyMetaDataKey) : FString();
    }

    static FCachedLevelState& Load(ULevel* Level)
    {
        FCachedLevelState& State = Cache.FindOrAdd(TWeakObjectPtr<ULevel>(Level));
        bool bPersisted = false;
        const FString Persisted = ReadPersistedValue(Level, bPersisted);
        if (!State.bLoaded || State.SourceValue != Persisted || State.bPersisted != bPersisted)
        {
            State.SourceValue = Persisted;
            State.bPersisted = bPersisted;
            State.bLoaded = true;
            Deserialize(Persisted, State.ChildrenByParent);
        }
        return State;
    }

    static void ApplySerialized(ULevel* Level, const FString& Value, bool bHasValue)
    {
        if (!Level || !Level->GetPackage())
        {
            return;
        }
        UPackage* Package = Level->GetPackage();
        FMetaData& Meta = Package->GetMetaData();
        if (bHasValue)
        {
            Meta.SetValue(Level, HierarchyMetaDataKey, *Value);
        }
        else
        {
            Meta.RemoveValue(Level, HierarchyMetaDataKey);
        }
        Package->MarkPackageDirty();
        Cache.Remove(TWeakObjectPtr<ULevel>(Level));
    }

    class FHierarchyChange final : public FCommandChange
    {
    public:
        FHierarchyChange(bool bInBeforeHad, FString InBefore, bool bInAfterHad, FString InAfter)
            : bBeforeHad(bInBeforeHad)
            , Before(MoveTemp(InBefore))
            , bAfterHad(bInAfterHad)
            , After(MoveTemp(InAfter))
        {
        }

        virtual void Apply(UObject* Object) override
        {
            ApplySerialized(Cast<ULevel>(Object), After, bAfterHad);
        }

        virtual void Revert(UObject* Object) override
        {
            ApplySerialized(Cast<ULevel>(Object), Before, bBeforeHad);
        }

        virtual FString ToString() const override { return TEXT("Index hierarchy order"); }

    private:
        bool bBeforeHad = false;
        FString Before;
        bool bAfterHad = false;
        FString After;
    };

    class FNativeHierarchyUndoBaselineChange final : public FCommandChange
    {
    public:
        FNativeHierarchyUndoBaselineChange(bool bInHadValue, FString InValue)
            : bHadValue(bInHadValue)
            , Value(MoveTemp(InValue))
        {
        }

        virtual void Apply(UObject* Object) override
        {
            ULevel* Level = Cast<ULevel>(Object);

            // Redo does not use the Undo-only transaction restore candidates. Clear
            // any candidate left by the preceding Undo before restoring the baseline.
            ClearUndoRestoreSnapshots(Level);

            // Redo first restores the same pre-command Index baseline. The native
            // hierarchy is then already in its redone state, so PostUndoRedo's
            // Outliner refresh deterministically derives the post-command ordering
            // from this baseline instead of replaying a stale deferred snapshot.
            ApplySerialized(Level, Value, bHadValue);
        }

        virtual void Revert(UObject* Object) override
        {
            ULevel* Level = Cast<ULevel>(Object);

            // Preserve the exact hierarchy sequence attached to this native Unreal
            // transaction. The post-Undo Outliner refresh can then restore returning
            // siblings from the transaction that actually moved them rather than
            // relying on session history whose local indices may have compressed.
            RecordUndoRestoreSnapshot(Level, Value);
            ApplySerialized(Level, Value, bHadValue);
        }

        virtual FString ToString() const override { return TEXT("Index native hierarchy undo baseline"); }

    private:
        bool bHadValue = false;
        FString Value;
    };

    static void CaptureNativeHierarchyUndoBaseline(ULevel* Level)
    {
        // GUndo is present while Unreal is recording a scoped editor transaction.
        // GIsTransacting is reserved here for the actual Undo/Redo replay, where we
        // must never add a new transaction record.
        if (!Level || !GUndo || GIsTransacting)
        {
            return;
        }

        FNativeMoveBaseline& Baseline =
            NativeMoveBaselines.FindOrAdd(TWeakObjectPtr<ULevel>(Level));

        // Create Folder with Selection and similar native hierarchy commands may be
        // split into several consecutive Unreal transactions even though the user
        // performed one command. Deferred Outliner reconciliation can update Index
        // metadata between those transactions. If we read the persisted value every
        // time, later transactions capture a partial state such as [A] instead of the
        // true pre-command [A,B], causing B to return ahead of A on Undo.
        //
        // Native callbacks for one command are synchronous or at most spill through a
        // couple of Outliner refresh frames. Share one immutable undo anchor across a
        // small frame window so every transaction in that burst records the exact same
        // pre-command Index state. A separate user command naturally occurs after the
        // window and captures a fresh baseline.
        const bool bAnchorExpired =
            !Baseline.bUndoAnchorValid
            || GFrameCounter > Baseline.UndoAnchorFrame + 4;

        if (bAnchorExpired)
        {
            Baseline.UndoAnchorFrame = GFrameCounter;
            Baseline.bUndoAnchorValid = true;
            Baseline.UndoAnchorValue = ReadPersistedValue(Level, Baseline.bUndoAnchorHadValue);
        }

        GUndo->StoreUndo(
            Level,
            MakeUnique<FNativeHierarchyUndoBaselineChange>(
                Baseline.bUndoAnchorHadValue,
                Baseline.UndoAnchorValue));
    }

    static bool Save(ULevel* Level, FCachedLevelState& State)
    {
        if (!Level || !Level->GetPackage())
        {
            return false;
        }

        const FString After = Serialize(State.ChildrenByParent);
        bool bBeforeHad = false;
        const FString Before = ReadPersistedValue(Level, bBeforeHad);
        if (bBeforeHad && Before == After)
        {
            State.SourceValue = After;
            State.bPersisted = true;
            return false;
        }

        if (GUndo && !GIsTransacting)
        {
            GUndo->StoreUndo(Level, MakeUnique<FHierarchyChange>(bBeforeHad, Before, true, After));
        }

        UPackage* Package = Level->GetPackage();
        FMetaData& Meta = Package->GetMetaData();
        Meta.SetValue(Level, HierarchyMetaDataKey, *After);
        Package->MarkPackageDirty();
        State.SourceValue = After;
        State.bPersisted = true;
        State.bLoaded = true;
        return true;
    }

    static void RemoveIdEverywhere(FCachedLevelState& State, const FString& Id)
    {
        if (Id.IsEmpty())
        {
            return;
        }
        for (TPair<FString, TArray<FString>>& Pair : State.ChildrenByParent)
        {
            Pair.Value.Remove(Id);
        }
    }

    static void ReplaceIdEverywhere(FCachedLevelState& State, const FString& OldId, const FString& NewId)
    {
        if (OldId.IsEmpty() || NewId.IsEmpty() || OldId == NewId)
        {
            return;
        }
        for (TPair<FString, TArray<FString>>& Pair : State.ChildrenByParent)
        {
            for (FString& Child : Pair.Value)
            {
                if (Child == OldId)
                {
                    Child = NewId;
                }
            }
        }
        if (TArray<FString>* Children = State.ChildrenByParent.Find(OldId))
        {
            TArray<FString> MovedChildren = *Children;
            State.ChildrenByParent.Remove(OldId);
            State.ChildrenByParent.Add(NewId, MoveTemp(MovedChildren));
        }
    }

    static bool ResolveFolderById(ULevel* Level, const FString& Id, FFolder& OutFolder)
    {
        if (!Level || !Level->GetWorld())
        {
            return false;
        }

        bool bFound = false;
        FActorFolders::Get().ForEachFolder(*Level->GetWorld(), [&](const FFolder& Folder)
        {
            if (Folder.IsValid()
                && Folder.GetRootObjectAssociatedLevel() == Level
                && FolderId(Folder) == Id)
            {
                OutFolder = Folder;
                bFound = true;
                return false;
            }
            return true;
        });
        return bFound;
    }

    static FString DisplayName(const FIndexSequenceItem& Item)
    {
        if (Item.IsActor())
        {
            return Item.Actor.IsValid() ? Item.Actor->GetActorLabel() : FString();
        }
        return Item.Folder.IsValid() ? Item.Folder.GetLeafName().ToString() : FString();
    }

    static bool CanMoveActorToGroup(AActor* Actor, const FIndexSequenceGroupKey& TargetGroup)
    {
        if (!IsValid(Actor) || !TargetGroup.IsValid() || Actor->GetLevel() != TargetGroup.Level.Get())
        {
            return false;
        }
        if (TargetGroup.bAttachedActorTier)
        {
            AActor* TargetParent = TargetGroup.ParentActor.Get();
            return IsValid(TargetParent) && TargetParent != Actor && !TargetParent->IsAttachedTo(Actor);
        }
        return Actor->GetFolderRootObject() == TargetGroup.RootObject;
    }

    static void MoveActorToGroup(AActor* Actor, const FIndexSequenceGroupKey& TargetGroup)
    {
        if (!IsValid(Actor))
        {
            return;
        }
        Actor->Modify();
        if (TargetGroup.bAttachedActorTier)
        {
            AActor* Parent = TargetGroup.ParentActor.Get();
            if (IsValid(Parent) && Actor->GetAttachParentActor() != Parent)
            {
                Parent->Modify(false);
                Actor->AttachToActor(Parent, FAttachmentTransformRules::KeepWorldTransform);
            }
            return;
        }
        if (AActor* PreviousParent = Actor->GetAttachParentActor())
        {
            PreviousParent->Modify(false);
            Actor->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        }
        if (Actor->GetFolderPath() != TargetGroup.FolderPath)
        {
            Actor->SetFolderPath_Recursively(TargetGroup.FolderPath);
        }
    }

    static FName BuildFolderPath(const FName& ParentPath, const FName& LeafName)
    {
        return ParentPath.IsNone()
            ? LeafName
            : FName(*(ParentPath.ToString() / LeafName.ToString()));
    }

    static bool ResolveFolderByPath(UWorld& World, const FFolder::FRootObject& RootObject, const FName& Path, FFolder& OutFolder)
    {
        bool bFound = false;
        FActorFolders::Get().ForEachFolder(World, [&](const FFolder& Folder)
        {
            if (Folder.IsValid() && Folder.GetRootObject() == RootObject && Folder.GetPath() == Path)
            {
                OutFolder = Folder;
                bFound = true;
                return false;
            }
            return true;
        });
        return bFound;
    }

    static bool CanMoveFolderToGroup(const FFolder& Folder, const FIndexSequenceGroupKey& TargetGroup)
    {
        if (!Folder.IsValid() || !TargetGroup.IsValid() || TargetGroup.bAttachedActorTier
            || Folder.GetRootObjectAssociatedLevel() != TargetGroup.Level.Get()
            || Folder.GetRootObject() != TargetGroup.RootObject)
        {
            return false;
        }

        const FString FolderPath = Folder.GetPath().ToString();
        const FString DestinationParentPath = TargetGroup.FolderPath.ToString();
        if (DestinationParentPath == FolderPath
            || (!FolderPath.IsEmpty() && DestinationParentPath.StartsWith(FolderPath + TEXT("/"))))
        {
            return false;
        }

        const FName NewPath = BuildFolderPath(TargetGroup.FolderPath, Folder.GetLeafName());
        if (NewPath == Folder.GetPath())
        {
            return true;
        }

        ULevel* Level = TargetGroup.Level.Get();
        UWorld* World = Level ? Level->GetWorld() : nullptr;
        return World && !FActorFolders::Get().ContainsFolder(*World, FFolder(TargetGroup.RootObject, NewPath));
    }

    static bool MoveFolderToGroup(const FFolder& Folder, const FIndexSequenceGroupKey& TargetGroup, FFolder& OutMovedFolder)
    {
        OutMovedFolder = Folder;
        if (!CanMoveFolderToGroup(Folder, TargetGroup))
        {
            return false;
        }
        const FName NewPath = BuildFolderPath(TargetGroup.FolderPath, Folder.GetLeafName());
        if (NewPath == Folder.GetPath())
        {
            return true;
        }
        ULevel* Level = TargetGroup.Level.Get();
        UWorld* World = Level ? Level->GetWorld() : nullptr;
        if (!World || !FActorFolders::Get().RenameFolderInWorld(*World, Folder, FFolder(TargetGroup.RootObject, NewPath)))
        {
            return false;
        }
        return ResolveFolderByPath(*World, TargetGroup.RootObject, NewPath, OutMovedFolder);
    }

    static bool IsDescendantOf(const FIndexSequenceItem& Candidate, const FIndexSequenceItem& Ancestor)
    {
        if (!Candidate.IsValid() || !Ancestor.IsValid() || Candidate == Ancestor)
        {
            return false;
        }

        if (Ancestor.IsActor())
        {
            if (!Candidate.IsActor())
            {
                return false;
            }
            for (AActor* Parent = Candidate.Actor->GetAttachParentActor(); Parent; Parent = Parent->GetAttachParentActor())
            {
                if (Parent == Ancestor.Actor.Get())
                {
                    return true;
                }
            }
            return false;
        }

        const FFolder AncestorFolder = Ancestor.Folder;
        if (Candidate.IsFolder())
        {
            return Candidate.Folder.IsChildOf(AncestorFolder);
        }

        AActor* VisualActor = Candidate.Actor.Get();
        while (VisualActor && VisualActor->GetAttachParentActor())
        {
            VisualActor = VisualActor->GetAttachParentActor();
        }
        if (!VisualActor)
        {
            return false;
        }
        const FFolder VisualFolder = VisualActor->GetFolder();
        return VisualFolder.IsValid()
            && (VisualFolder == AncestorFolder || VisualFolder.IsChildOf(AncestorFolder));
    }

    static void EnsureGroupFromNative(const FIndexSequenceItem& Item)
    {
        TArray<FIndexSequenceItem> Siblings = FIndexSequenceState::GetSiblings(Item);
        FIndexSequenceState::EnsureCaptured(Siblings);
    }

    static FString GroupParentId(const FIndexSequenceGroupKey& Group)
    {
        ULevel* Level = Group.Level.Get();
        if (!Level)
        {
            return FString();
        }
        if (Group.bAttachedActorTier)
        {
            return ActorId(Group.ParentActor.Get());
        }
        if (!Group.FolderPath.IsNone())
        {
            FFolder Folder;
            if (UWorld* World = Level->GetWorld())
            {
                if (ResolveFolderByPath(*World, Group.RootObject, Group.FolderPath, Folder))
                {
                    return FolderId(Folder);
                }
            }
            return FString();
        }
        return RootId(Group.RootObject, Level);
    }

    static TArray<FIndexSequenceItem> GetItemsInGroup(const FIndexSequenceGroupKey& Group)
    {
        TArray<FIndexSequenceItem> Result;
        if (!Group.IsValid())
        {
            return Result;
        }

        if (Group.bAttachedActorTier)
        {
            if (AActor* Parent = Group.ParentActor.Get())
            {
                TArray<AActor*> Children;
                Parent->GetAttachedActors(Children, false);
                for (AActor* Child : Children)
                {
                    if (IsValid(Child) && Child->GetLevel() == Group.Level.Get())
                    {
                        Result.Add(FIndexSequenceItem::FromActor(Child));
                    }
                }
            }
            return Result;
        }

        ULevel* Level = Group.Level.Get();
        if (!Level)
        {
            return Result;
        }

        for (AActor* Actor : Level->Actors)
        {
            if (!IsValid(Actor) || Actor->GetAttachParentActor())
            {
                continue;
            }
            const FIndexSequenceItem Candidate = FIndexSequenceItem::FromActor(Actor);
            if (FIndexSequenceState::GetGroupKey(Candidate) == Group)
            {
                Result.Add(Candidate);
            }
        }

        if (UWorld* World = Level->GetWorld())
        {
            FActorFolders::Get().ForEachFolder(*World, [&](const FFolder& Folder)
            {
                if (Folder.IsValid())
                {
                    const FIndexSequenceItem Candidate = FIndexSequenceItem::FromFolder(Folder);
                    if (FIndexSequenceState::GetGroupKey(Candidate) == Group)
                    {
                        Result.Add(Candidate);
                    }
                }
                return true;
            });
        }
        return Result;
    }

    static TArray<int32> VisualPath(const FIndexSequenceItem& Item)
    {
        TArray<int32> Reverse;
        FIndexSequenceItem Current = Item;
        int32 Guard = 0;
        while (Current.IsValid() && Guard++ < 64)
        {
            EnsureGroupFromNative(Current);
            Reverse.Add(FIndexSequenceState::GetOrder(Current));

            if (Current.IsActor())
            {
                AActor* Actor = Current.Actor.Get();
                if (AActor* ParentActor = Actor->GetAttachParentActor())
                {
                    Current = FIndexSequenceItem::FromActor(ParentActor);
                    continue;
                }
                const FFolder Folder = Actor->GetFolder();
                Current = (Folder.IsValid() && !Folder.IsNone())
                    ? FIndexSequenceItem::FromFolder(Folder)
                    : FIndexSequenceItem();
            }
            else
            {
                const FFolder Parent = Current.Folder.GetParent();
                Current = (Parent.IsValid() && !Parent.IsNone())
                    ? FIndexSequenceItem::FromFolder(Parent)
                    : FIndexSequenceItem();
            }
        }

        Algo::Reverse(Reverse);
        return Reverse;
    }
}

FIndexSequenceItem FIndexSequenceItem::FromActor(AActor* InActor)
{
    FIndexSequenceItem Item;
    Item.Type = EIndexSequenceItemType::Actor;
    Item.Actor = InActor;
    return Item;
}

FIndexSequenceItem FIndexSequenceItem::FromFolder(const FFolder& InFolder)
{
    FIndexSequenceItem Item;
    Item.Type = EIndexSequenceItemType::Folder;
    Item.Folder = InFolder;
    return Item;
}

bool FIndexSequenceItem::IsValid() const
{
    return IsActor() ? Actor.IsValid() : Folder.IsValid();
}

bool FIndexSequenceItem::operator==(const FIndexSequenceItem& Other) const
{
    if (Type != Other.Type)
    {
        return false;
    }
    if (IsActor())
    {
        return Actor == Other.Actor;
    }
    if (!Folder.IsValid() || !Other.Folder.IsValid())
    {
        return Folder == Other.Folder;
    }
    const FGuid& A = Folder.GetActorFolderGuid();
    const FGuid& B = Other.Folder.GetActorFolderGuid();
    return A.IsValid() && B.IsValid() ? A == B : Folder == Other.Folder;
}

FIndexSequenceGroupKey FIndexSequenceState::GetGroupKey(const FIndexSequenceItem& Item)
{
    FIndexSequenceGroupKey Key;
    if (!Item.IsValid())
    {
        return Key;
    }

    if (Item.IsActor())
    {
        AActor* Actor = Item.Actor.Get();
        Key.Level = Actor->GetLevel();
        if (AActor* Parent = Actor->GetAttachParentActor())
        {
            Key.bAttachedActorTier = true;
            Key.ParentActor = Parent;
        }
        else
        {
            Key.RootObject = Actor->GetFolderRootObject();
            Key.FolderPath = Actor->GetFolderPath();
            const FFolder Folder = Actor->GetFolder();
            if (Folder.IsValid() && !Folder.IsNone())
            {
                Key.ParentFolderGuid = Folder.GetActorFolderGuid();
            }
        }
        return Key;
    }

    Key.Level = Item.Folder.GetRootObjectAssociatedLevel();
    Key.RootObject = Item.Folder.GetRootObject();
    const FFolder Parent = Item.Folder.GetParent();
    if (Parent.IsValid() && !Parent.IsNone())
    {
        Key.FolderPath = Parent.GetPath();
        Key.ParentFolderGuid = Parent.GetActorFolderGuid();
    }
    return Key;
}

TArray<FIndexSequenceItem> FIndexSequenceState::GetSiblings(const FIndexSequenceItem& Item)
{
    TArray<FIndexSequenceItem> Result;
    const FIndexSequenceGroupKey Group = GetGroupKey(Item);
    if (!Group.IsValid())
    {
        return Result;
    }

    if (Group.bAttachedActorTier)
    {
        if (AActor* Parent = Group.ParentActor.Get())
        {
            TArray<AActor*> Children;
            Parent->GetAttachedActors(Children, false);
            for (AActor* Child : Children)
            {
                if (IsValid(Child) && Child->GetLevel() == Group.Level.Get())
                {
                    Result.Add(FIndexSequenceItem::FromActor(Child));
                }
            }
        }
        return Result;
    }

    ULevel* Level = Group.Level.Get();
    if (!Level)
    {
        return Result;
    }

    for (AActor* Actor : Level->Actors)
    {
        if (!IsValid(Actor) || Actor->GetAttachParentActor())
        {
            continue;
        }
        const FIndexSequenceItem Candidate = FIndexSequenceItem::FromActor(Actor);
        if (GetGroupKey(Candidate) == Group)
        {
            Result.Add(Candidate);
        }
    }

    if (UWorld* World = Level->GetWorld())
    {
        FActorFolders::Get().ForEachFolder(*World, [&](const FFolder& Folder)
        {
            if (Folder.IsValid())
            {
                const FIndexSequenceItem Candidate = FIndexSequenceItem::FromFolder(Folder);
                if (GetGroupKey(Candidate) == Group)
                {
                    Result.Add(Candidate);
                }
            }
            return true;
        });
    }
    return Result;
}

void FIndexSequenceState::EnsureCaptured(const TArray<FIndexSequenceItem>& ItemsInCurrentOrder)
{
    FIndexSequenceItem First;
    for (const FIndexSequenceItem& Item : ItemsInCurrentOrder)
    {
        if (Item.IsValid())
        {
            First = Item;
            break;
        }
    }
    if (!First.IsValid())
    {
        return;
    }

    ULevel* Level = IndexSequenceStatePrivate::GetLevel(First);
    if (!Level)
    {
        return;
    }

    // Never reconcile a partially restored hierarchy while Unreal is applying an
    // Undo/Redo transaction. PostUndoRedo invalidates the cache and refreshes the
    // Outliner once the native hierarchy and Index metadata have both settled.
    if (GIsTransacting)
    {
        return;
    }

    const FString Parent = IndexSequenceStatePrivate::ParentId(First);
    if (Parent.IsEmpty())
    {
        return;
    }

    IndexSequenceStatePrivate::FCachedLevelState& State = IndexSequenceStatePrivate::Load(Level);
    IndexSequenceStatePrivate::FNativeMoveBaseline& Baseline =
        IndexSequenceStatePrivate::GetNativeMoveBaseline(Level, State);

    struct FCaptureCandidate
    {
        FString Id;
        FString PreviousParent;
        TArray<int32> PreviousVisualPath;
        int32 NativeIndex = 0;
        bool bWasPreviouslyTracked = false;

        bool WasPreviouslyTracked() const { return bWasPreviouslyTracked; }
    };

    const bool bPostUndoRedoRestore = IndexSequenceStatePrivate::IsPostUndoRedoRestoreWindow();

    if (bPostUndoRedoRestore)
    {
        TArray<FString> CurrentIds;
        CurrentIds.Reserve(ItemsInCurrentOrder.Num());
        for (const FIndexSequenceItem& Item : ItemsInCurrentOrder)
        {
            if (!Item.IsValid()
                || IndexSequenceStatePrivate::GetLevel(Item) != Level
                || IndexSequenceStatePrivate::ParentId(Item) != Parent)
            {
                continue;
            }

            const FString Id = IndexSequenceStatePrivate::ItemId(Item);
            if (!Id.IsEmpty() && !CurrentIds.Contains(Id))
            {
                CurrentIds.Add(Id);
            }
        }

        TArray<FString> DepartureSequence;
        if (IndexSequenceStatePrivate::TryGetDepartureRestoreSequence(
                Level, Parent, CurrentIds, DepartureSequence))
        {
            // A row returning during Undo carries an exact snapshot of the source
            // sibling tier from the forward move that removed it. Once the complete
            // old membership is present again, restore that sequence atomically.
            // This avoids ambiguous per-item slot history after the source tier was
            // temporarily compressed (for example [G,H] becoming [H]).
            for (const FString& Id : CurrentIds)
            {
                IndexSequenceStatePrivate::RemoveIdEverywhere(State, Id);
            }
            State.ChildrenByParent.Add(Parent, MoveTemp(DepartureSequence));
            IndexSequenceStatePrivate::Save(Level, State);
            return;
        }

        TArray<FString> TransactionSequence;
        if (IndexSequenceStatePrivate::TryGetTransactionUndoSequence(
                Level, Parent, CurrentIds, TransactionSequence))
        {
            // The transaction snapshot is authoritative only when the complete old
            // sibling membership has returned. Remove those IDs from whatever
            // temporary tiers the forward operation recorded, then restore the exact
            // pre-change local sequence in one step.
            for (const FString& Id : CurrentIds)
            {
                IndexSequenceStatePrivate::RemoveIdEverywhere(State, Id);
            }
            State.ChildrenByParent.Add(Parent, MoveTemp(TransactionSequence));
            IndexSequenceStatePrivate::Save(Level, State);
            return;
        }
    }

    auto MakeCandidate = [&Baseline, bPostUndoRedoRestore, &Parent](const FString& Id, int32 NativeIndex)
    {
        FCaptureCandidate Candidate;
        Candidate.Id = Id;
        Candidate.NativeIndex = NativeIndex;
        if (const TArray<int32>* PreviousPath = Baseline.VisualPathById.Find(Id))
        {
            Candidate.PreviousVisualPath = *PreviousPath;
            Candidate.bWasPreviouslyTracked = true;
        }
        if (const FString* PreviousParent = Baseline.ParentById.Find(Id))
        {
            Candidate.PreviousParent = *PreviousParent;
        }

        // A short-lived native baseline may have expired long before the user presses
        // Undo. During the explicit post-Undo/Redo refresh only, prefer the durable
        // session history for the current parent. This is what lets [A] + returning B
        // recover B's old slot after A even if the current persisted metadata still
        // describes B as living in the temporary destination folder.
        if (bPostUndoRedoRestore)
        {
            if (const TMap<FString, TArray<int32>>* HistoryByParent =
                    Baseline.HistoricalVisualPathByIdAndParent.Find(Id))
            {
                if (const TArray<int32>* HistoricalPath = HistoryByParent->Find(Parent))
                {
                    Candidate.PreviousParent = Parent;
                    Candidate.PreviousVisualPath = *HistoricalPath;
                    Candidate.bWasPreviouslyTracked = true;
                }
            }
        }
        return Candidate;
    };

    auto SortCandidatesForNativeMove = [](TArray<FCaptureCandidate>& Candidates)
    {
        Candidates.StableSort([](const FCaptureCandidate& A, const FCaptureCandidate& B)
        {
            // Brand-new rows retain Unreal's current order and remain ahead of
            // previously tracked rows, preserving Index's "new content at top" rule.
            if (A.WasPreviouslyTracked() != B.WasPreviouslyTracked())
            {
                return !A.WasPreviouslyTracked();
            }
            if (!A.WasPreviouslyTracked())
            {
                return A.NativeIndex < B.NativeIndex;
            }

            if (IndexSequenceStatePrivate::StoredPathLess(A.PreviousVisualPath, B.PreviousVisualPath))
            {
                return true;
            }
            if (IndexSequenceStatePrivate::StoredPathLess(B.PreviousVisualPath, A.PreviousVisualPath))
            {
                return false;
            }
            return A.NativeIndex < B.NativeIndex;
        });
    };

    TArray<FString>* Existing = State.ChildrenByParent.Find(Parent);
    if (!Existing)
    {
        TArray<FCaptureCandidate> Candidates;
        int32 NativeIndex = 0;
        for (const FIndexSequenceItem& Item : ItemsInCurrentOrder)
        {
            if (Item.IsValid()
                && IndexSequenceStatePrivate::GetLevel(Item) == Level
                && IndexSequenceStatePrivate::ParentId(Item) == Parent)
            {
                const FString Id = IndexSequenceStatePrivate::ItemId(Item);
                if (!Id.IsEmpty() && !Candidates.ContainsByPredicate([&Id](const FCaptureCandidate& Candidate) { return Candidate.Id == Id; }))
                {
                    Candidates.Add(MakeCandidate(Id, NativeIndex));
                }
            }
            ++NativeIndex;
        }

        SortCandidatesForNativeMove(Candidates);

        TArray<FString> Captured;
        Captured.Reserve(Candidates.Num());

        // Snapshot every source tier before removing any member. Capturing the whole
        // tier first is important for multi-row moves: removing one selected row must
        // not compress the source sequence before the next selected row records it.
        if (!bPostUndoRedoRestore)
        {
            for (const FCaptureCandidate& Candidate : Candidates)
            {
                if (Candidate.WasPreviouslyTracked())
                {
                    IndexSequenceStatePrivate::CaptureDepartureSequenceFromState(
                        Level, State, Candidate.Id);
                }
            }
        }

        for (const FCaptureCandidate& Candidate : Candidates)
        {
            // If this is a newly created destination tier containing rows that were
            // already tracked elsewhere, consume their old locations now. The
            // immutable baseline still retains every row's pre-move traversal path.
            IndexSequenceStatePrivate::RemoveIdEverywhere(State, Candidate.Id);
            Captured.Add(Candidate.Id);
        }

        State.ChildrenByParent.Add(Parent, MoveTemp(Captured));
        // Index owns the complete local sibling sequence from the moment the tier is
        // first seen. Persist that baseline so Undo can always restore the exact
        // pre-edit order instead of trying to reconstruct it from labels later.
        IndexSequenceStatePrivate::Save(Level, State);
        return;
    }

    TArray<FCaptureCandidate> Missing;
    int32 NativeIndex = 0;
    for (const FIndexSequenceItem& Item : ItemsInCurrentOrder)
    {
        if (!Item.IsValid() || IndexSequenceStatePrivate::ParentId(Item) != Parent)
        {
            ++NativeIndex;
            continue;
        }
        const FString Id = IndexSequenceStatePrivate::ItemId(Item);
        if (!Id.IsEmpty()
            && !Existing->Contains(Id)
            && !Missing.ContainsByPredicate([&Id](const FCaptureCandidate& Candidate) { return Candidate.Id == Id; }))
        {
            Missing.Add(MakeCandidate(Id, NativeIndex));
        }
        ++NativeIndex;
    }

    if (!Missing.IsEmpty())
    {
        // Preserve the exact source sibling tiers before removing any arriving row.
        // These full sequences are used only during post-Undo reconciliation and do
        // not depend on frame timing or transaction grouping.
        if (!bPostUndoRedoRestore)
        {
            for (const FCaptureCandidate& Candidate : Missing)
            {
                if (Candidate.WasPreviouslyTracked())
                {
                    IndexSequenceStatePrivate::CaptureDepartureSequenceFromState(
                        Level, State, Candidate.Id);
                }
            }
        }

        // Remove newly arrived rows from their old canonical tiers first. Do not
        // decide their destination indices yet: rows from the same native operation
        // may already have arrived in this tier during an earlier Outliner refresh.
        for (const FCaptureCandidate& Candidate : Missing)
        {
            IndexSequenceStatePrivate::RemoveIdEverywhere(State, Candidate.Id);
        }

        TArray<FString>& CurrentChildren = State.ChildrenByParent.FindOrAdd(Parent);

        TArray<FString> ExistingBrandNew;
        TArray<FCaptureCandidate> IncomingMovedTracked;
        TArray<FCaptureCandidate> LocalTracked;

        for (int32 Index = 0; Index < CurrentChildren.Num(); ++Index)
        {
            const FString& Id = CurrentChildren[Index];
            FCaptureCandidate Candidate = MakeCandidate(Id, Index);

            if (!Candidate.WasPreviouslyTracked())
            {
                // A genuinely new row that arrived earlier in this same short native
                // batch stays in the leading new-content block.
                ExistingBrandNew.Add(Id);
                continue;
            }

            if (Candidate.PreviousParent != Parent)
            {
                if (!IncomingMovedTracked.ContainsByPredicate([&Id](const FCaptureCandidate& ExistingCandidate) { return ExistingCandidate.Id == Id; }))
                {
                    IncomingMovedTracked.Add(MoveTemp(Candidate));
                }
            }
            else if (!LocalTracked.ContainsByPredicate([&Id](const FCaptureCandidate& ExistingCandidate) { return ExistingCandidate.Id == Id; }))
            {
                LocalTracked.Add(MoveTemp(Candidate));
            }
        }

        TArray<FCaptureCandidate> NewBrandNew;
        for (const FCaptureCandidate& Candidate : Missing)
        {
            if (!Candidate.WasPreviouslyTracked())
            {
                NewBrandNew.Add(Candidate);
            }
            else if (Candidate.PreviousParent == Parent)
            {
                // Undo/Redo can return a row to the same parent it occupied in the
                // immutable pre-move baseline. It is not an incoming item in that
                // case: merge it back with the siblings that never left, using the
                // historical visual paths for both sides. Treating every missing row
                // as incoming is what turned [A] + returning B into [B,A].
                if (!LocalTracked.ContainsByPredicate([&Candidate](const FCaptureCandidate& ExistingCandidate) { return ExistingCandidate.Id == Candidate.Id; }))
                {
                    LocalTracked.Add(Candidate);
                }
            }
            else if (!IncomingMovedTracked.ContainsByPredicate([&Candidate](const FCaptureCandidate& ExistingCandidate) { return ExistingCandidate.Id == Candidate.Id; }))
            {
                IncomingMovedTracked.Add(Candidate);
            }
        }

        SortCandidatesForNativeMove(NewBrandNew);
        SortCandidatesForNativeMove(IncomingMovedTracked);
        SortCandidatesForNativeMove(LocalTracked);

        TArray<FString> Rebuilt;
        Rebuilt.Reserve(ExistingBrandNew.Num() + NewBrandNew.Num() + IncomingMovedTracked.Num() + LocalTracked.Num());
        Rebuilt.Append(ExistingBrandNew);
        for (const FCaptureCandidate& Candidate : NewBrandNew)
        {
            Rebuilt.Add(Candidate.Id);
        }
        for (const FCaptureCandidate& Candidate : IncomingMovedTracked)
        {
            Rebuilt.Add(Candidate.Id);
        }
        for (const FCaptureCandidate& Candidate : LocalTracked)
        {
            Rebuilt.Add(Candidate.Id);
        }

        CurrentChildren = MoveTemp(Rebuilt);
        IndexSequenceStatePrivate::Save(Level, State);
    }
}

bool FIndexSequenceState::HasOrder(const FIndexSequenceItem& Item)
{
    return GetOrder(Item) != MAX_int32;
}

int32 FIndexSequenceState::GetOrder(const FIndexSequenceItem& Item)
{
    if (!Item.IsValid())
    {
        return MAX_int32;
    }
    ULevel* Level = IndexSequenceStatePrivate::GetLevel(Item);
    const FString Parent = IndexSequenceStatePrivate::ParentId(Item);
    const FString Id = IndexSequenceStatePrivate::ItemId(Item);
    if (!Level || Parent.IsEmpty() || Id.IsEmpty())
    {
        return MAX_int32;
    }
    IndexSequenceStatePrivate::FCachedLevelState& State = IndexSequenceStatePrivate::Load(Level);
    if (const TArray<FString>* Children = State.ChildrenByParent.Find(Parent))
    {
        const int32 Index = Children->IndexOfByKey(Id);
        return Index == INDEX_NONE ? MAX_int32 : Index;
    }
    return MAX_int32;
}

void FIndexSequenceState::SetOrder(const FIndexSequenceItem& Item, int32 NewOrder)
{
    if (!Item.IsValid())
    {
        return;
    }
    TArray<FIndexSequenceItem> Siblings = GetSiblings(Item);
    EnsureCaptured(Siblings);
    SortItems(Siblings);
    Siblings.Remove(Item);
    const int32 Index = FMath::Clamp(NewOrder, 0, Siblings.Num());
    Siblings.Insert(Item, Index);

    ULevel* Level = IndexSequenceStatePrivate::GetLevel(Item);
    if (!Level)
    {
        return;
    }
    IndexSequenceStatePrivate::FCachedLevelState& State = IndexSequenceStatePrivate::Load(Level);
    TArray<FString>& Children = State.ChildrenByParent.FindOrAdd(IndexSequenceStatePrivate::ParentId(Item));
    Children.Reset();
    for (const FIndexSequenceItem& Sibling : Siblings)
    {
        Children.Add(IndexSequenceStatePrivate::ItemId(Sibling));
    }
    IndexSequenceStatePrivate::Save(Level, State);
}

bool FIndexSequenceState::Less(const FIndexSequenceItem& A, const FIndexSequenceItem& B)
{
    if (A == B)
    {
        return false;
    }
    if (!A.IsValid())
    {
        return false;
    }
    if (!B.IsValid())
    {
        return true;
    }

    const int32 OrderA = GetOrder(A);
    const int32 OrderB = GetOrder(B);
    if (OrderA != OrderB)
    {
        return OrderA < OrderB;
    }

    const int32 NameCompare = IndexSequenceStatePrivate::DisplayName(A).Compare(
        IndexSequenceStatePrivate::DisplayName(B), ESearchCase::IgnoreCase);
    if (NameCompare != 0)
    {
        return NameCompare < 0;
    }
    return IndexSequenceStatePrivate::ItemId(A).Compare(IndexSequenceStatePrivate::ItemId(B), ESearchCase::CaseSensitive) < 0;
}

void FIndexSequenceState::SortItems(TArray<FIndexSequenceItem>& InOutItems)
{
    InOutItems.RemoveAll([](const FIndexSequenceItem& Item) { return !Item.IsValid(); });
    if (!InOutItems.IsEmpty())
    {
        EnsureCaptured(InOutItems);
    }
    InOutItems.Sort([](const FIndexSequenceItem& A, const FIndexSequenceItem& B) { return Less(A, B); });
}

void FIndexSequenceState::CollapseToTopLevelRoots(TArray<FIndexSequenceItem>& InOutItems)
{
    InOutItems.RemoveAll([](const FIndexSequenceItem& Item) { return !Item.IsValid(); });
    InOutItems.RemoveAll([&InOutItems](const FIndexSequenceItem& Candidate)
    {
        for (const FIndexSequenceItem& Other : InOutItems)
        {
            if (Candidate != Other && IndexSequenceStatePrivate::IsDescendantOf(Candidate, Other))
            {
                return true;
            }
        }
        return false;
    });
}

void FIndexSequenceState::SortItemsForMove(TArray<FIndexSequenceItem>& InOutItems)
{
    CollapseToTopLevelRoots(InOutItems);
    InOutItems.Sort([](const FIndexSequenceItem& A, const FIndexSequenceItem& B)
    {
        const TArray<int32> PathA = IndexSequenceStatePrivate::VisualPath(A);
        const TArray<int32> PathB = IndexSequenceStatePrivate::VisualPath(B);
        const int32 Count = FMath::Min(PathA.Num(), PathB.Num());
        for (int32 Index = 0; Index < Count; ++Index)
        {
            if (PathA[Index] != PathB[Index])
            {
                return PathA[Index] < PathB[Index];
            }
        }
        if (PathA.Num() != PathB.Num())
        {
            return PathA.Num() < PathB.Num();
        }
        return IndexSequenceStatePrivate::ItemId(A).Compare(IndexSequenceStatePrivate::ItemId(B), ESearchCase::CaseSensitive) < 0;
    });
}

bool FIndexSequenceState::CanInsertItemsRelative(const TArray<FIndexSequenceItem>& InMovedItems, const FIndexSequenceItem& TargetItem)
{
    if (!TargetItem.IsValid() || InMovedItems.IsEmpty())
    {
        return false;
    }

    TArray<FIndexSequenceItem> MovedItems = InMovedItems;
    CollapseToTopLevelRoots(MovedItems);
    const FIndexSequenceGroupKey TargetGroup = GetGroupKey(TargetItem);
    if (!TargetGroup.IsValid())
    {
        return false;
    }

    TSet<FName> PlannedFolderPaths;
    for (const FIndexSequenceItem& Item : MovedItems)
    {
        if (!Item.IsValid() || Item == TargetItem
            || IndexSequenceStatePrivate::GetLevel(Item) != IndexSequenceStatePrivate::GetLevel(TargetItem)
            || IndexSequenceStatePrivate::IsDescendantOf(TargetItem, Item))
        {
            return false;
        }
        if (Item.IsFolder())
        {
            if (!IndexSequenceStatePrivate::CanMoveFolderToGroup(Item.Folder, TargetGroup))
            {
                return false;
            }
            const FName Planned = IndexSequenceStatePrivate::BuildFolderPath(TargetGroup.FolderPath, Item.Folder.GetLeafName());
            if (PlannedFolderPaths.Contains(Planned))
            {
                return false;
            }
            PlannedFolderPaths.Add(Planned);
        }
        else if (!IndexSequenceStatePrivate::CanMoveActorToGroup(Item.Actor.Get(), TargetGroup))
        {
            return false;
        }
    }
    return true;
}

static bool InsertItemsRelativeImpl(
    const TArray<FIndexSequenceItem>& InMovedItems,
    const FIndexSequenceItem& TargetItem,
    bool bInsertBefore,
    bool bCreateTransaction)
{
    TArray<FIndexSequenceItem> MovedItems = InMovedItems;
    FIndexSequenceState::CollapseToTopLevelRoots(MovedItems);
    FIndexSequenceState::SortItemsForMove(MovedItems);
    if (!FIndexSequenceState::CanInsertItemsRelative(MovedItems, TargetItem))
    {
        return false;
    }

    ULevel* Level = IndexSequenceStatePrivate::GetLevel(TargetItem);
    if (!Level)
    {
        return false;
    }

    TArray<FIndexSequenceItem> DestinationBeforeMove = FIndexSequenceState::GetSiblings(TargetItem);
    FIndexSequenceState::EnsureCaptured(DestinationBeforeMove);
    for (const FIndexSequenceItem& Item : MovedItems)
    {
        FIndexSequenceState::EnsureCaptured(FIndexSequenceState::GetSiblings(Item));
    }

    const FString TargetId = IndexSequenceStatePrivate::ItemId(TargetItem);
    const FString TargetParentId = IndexSequenceStatePrivate::ParentId(TargetItem);
    TArray<FString> MovedIds;
    for (const FIndexSequenceItem& Item : MovedItems)
    {
        MovedIds.Add(IndexSequenceStatePrivate::ItemId(Item));
    }

    IndexSequenceStatePrivate::FCachedLevelState& State = IndexSequenceStatePrivate::Load(Level);
    TArray<FString> Preview = State.ChildrenByParent.FindRef(TargetParentId);
    for (const FString& Id : MovedIds)
    {
        Preview.Remove(Id);
    }
    const int32 TargetIndex = Preview.IndexOfByKey(TargetId);
    if (TargetIndex == INDEX_NONE)
    {
        return false;
    }
    int32 InsertIndex = bInsertBefore ? TargetIndex : TargetIndex + 1;
    for (int32 Index = 0; Index < MovedIds.Num(); ++Index)
    {
        Preview.Insert(MovedIds[Index], FMath::Clamp(InsertIndex + Index, 0, Preview.Num()));
    }
    bool bAllAlreadyInTargetTier = true;
    for (const FIndexSequenceItem& Item : MovedItems)
    {
        bAllAlreadyInTargetTier &= IndexSequenceStatePrivate::ParentId(Item) == TargetParentId;
    }
    if (State.ChildrenByParent.FindRef(TargetParentId) == Preview && bAllAlreadyInTargetTier)
    {
        return false;
    }

    IndexSequenceStatePrivate::FScopedExplicitReorderGuard Guard;
    TUniquePtr<FScopedTransaction> Transaction;
    if (bCreateTransaction)
    {
        Transaction = MakeUnique<FScopedTransaction>(LOCTEXT("ReorderOutlinerItems", "Index: Reorder Outliner Items"));
    }

    // Remove the moved roots from their old ordered child lists before Unreal mutates
    // their hierarchy. Descendant lists remain untouched and move with their parent.
    for (const FString& Id : MovedIds)
    {
        IndexSequenceStatePrivate::RemoveIdEverywhere(State, Id);
    }

    const FIndexSequenceGroupKey TargetGroup = FIndexSequenceState::GetGroupKey(TargetItem);
    for (int32 MoveIndex = 0; MoveIndex < MovedItems.Num(); ++MoveIndex)
    {
        FIndexSequenceItem& Item = MovedItems[MoveIndex];
        if (FIndexSequenceState::GetGroupKey(Item) == TargetGroup)
        {
            continue;
        }
        if (Item.IsActor())
        {
            IndexSequenceStatePrivate::MoveActorToGroup(Item.Actor.Get(), TargetGroup);
        }
        else
        {
            const FString OldId = IndexSequenceStatePrivate::ItemId(Item);
            FFolder MovedFolder;
            if (!IndexSequenceStatePrivate::MoveFolderToGroup(Item.Folder, TargetGroup, MovedFolder))
            {
                return false;
            }
            Item = FIndexSequenceItem::FromFolder(MovedFolder);
            const FString NewId = IndexSequenceStatePrivate::ItemId(Item);
            if (OldId != NewId)
            {
                IndexSequenceStatePrivate::ReplaceIdEverywhere(State, OldId, NewId);
                for (FString& MovedId : MovedIds)
                {
                    if (MovedId == OldId)
                    {
                        MovedId = NewId;
                    }
                }
                // Preview was built before Unreal reparents the folder. Some folder
                // moves rebuild the backing folder identity, so keep the pending
                // destination list in sync with the new stable ID as well. Otherwise
                // the next Outliner capture sees the moved folder as missing and
                // inserts it at the top of the destination tier.
                for (FString& PreviewId : Preview)
                {
                    if (PreviewId == OldId)
                    {
                        PreviewId = NewId;
                    }
                }
            }
        }
    }

    TArray<FString>& Destination = State.ChildrenByParent.FindOrAdd(TargetParentId);
    Destination = Preview;

    return IndexSequenceStatePrivate::Save(Level, State);
}

bool FIndexSequenceState::InsertItemsRelative(const TArray<FIndexSequenceItem>& MovedItems, const FIndexSequenceItem& TargetItem, bool bInsertBefore)
{
    return InsertItemsRelativeImpl(MovedItems, TargetItem, bInsertBefore, true);
}

bool FIndexSequenceState::InsertItemsRelativeInCurrentTransaction(const TArray<FIndexSequenceItem>& MovedItems, const FIndexSequenceItem& TargetItem, bool bInsertBefore)
{
    return InsertItemsRelativeImpl(MovedItems, TargetItem, bInsertBefore, false);
}

static bool MoveItemsToGroupImpl(
    const TArray<FIndexSequenceItem>& InMovedItems,
    const FIndexSequenceGroupKey& TargetGroup,
    bool bAtTop)
{
    if (!TargetGroup.IsValid() || InMovedItems.IsEmpty())
    {
        return false;
    }

    TArray<FIndexSequenceItem> MovedItems = InMovedItems;
    FIndexSequenceState::CollapseToTopLevelRoots(MovedItems);
    FIndexSequenceState::SortItemsForMove(MovedItems);
    if (MovedItems.IsEmpty())
    {
        return false;
    }

    ULevel* Level = TargetGroup.Level.Get();
    if (!Level)
    {
        return false;
    }

    TSet<FName> PlannedFolderPaths;
    for (const FIndexSequenceItem& Item : MovedItems)
    {
        if (IndexSequenceStatePrivate::GetLevel(Item) != Level)
        {
            return false;
        }
        if (Item.IsFolder())
        {
            if (!IndexSequenceStatePrivate::CanMoveFolderToGroup(Item.Folder, TargetGroup))
            {
                return false;
            }
            const FName Planned = IndexSequenceStatePrivate::BuildFolderPath(
                TargetGroup.FolderPath, Item.Folder.GetLeafName());
            if (PlannedFolderPaths.Contains(Planned))
            {
                return false;
            }
            PlannedFolderPaths.Add(Planned);
        }
        else if (!IndexSequenceStatePrivate::CanMoveActorToGroup(Item.Actor.Get(), TargetGroup))
        {
            return false;
        }
    }

    TArray<FIndexSequenceItem> DestinationBeforeMove =
        IndexSequenceStatePrivate::GetItemsInGroup(TargetGroup);
    if (!DestinationBeforeMove.IsEmpty())
    {
        FIndexSequenceState::EnsureCaptured(DestinationBeforeMove);
    }
    for (const FIndexSequenceItem& Item : MovedItems)
    {
        FIndexSequenceState::EnsureCaptured(FIndexSequenceState::GetSiblings(Item));
    }

    IndexSequenceStatePrivate::FCachedLevelState& State = IndexSequenceStatePrivate::Load(Level);
    const FString TargetParentId = IndexSequenceStatePrivate::GroupParentId(TargetGroup);
    if (TargetParentId.IsEmpty())
    {
        return false;
    }

    TArray<FString> MovedIds;
    MovedIds.Reserve(MovedItems.Num());
    for (const FIndexSequenceItem& Item : MovedItems)
    {
        const FString Id = IndexSequenceStatePrivate::ItemId(Item);
        if (Id.IsEmpty())
        {
            return false;
        }
        MovedIds.Add(Id);
    }

    IndexSequenceStatePrivate::FScopedExplicitReorderGuard Guard;
    const FScopedTransaction Transaction(LOCTEXT("MoveOutlinerBlock", "Index: Move Outliner Items"));

    for (const FString& Id : MovedIds)
    {
        IndexSequenceStatePrivate::RemoveIdEverywhere(State, Id);
    }

    for (int32 MoveIndex = 0; MoveIndex < MovedItems.Num(); ++MoveIndex)
    {
        FIndexSequenceItem& Item = MovedItems[MoveIndex];
        if (FIndexSequenceState::GetGroupKey(Item) == TargetGroup)
        {
            continue;
        }

        if (Item.IsActor())
        {
            IndexSequenceStatePrivate::MoveActorToGroup(Item.Actor.Get(), TargetGroup);
        }
        else
        {
            const FString OldId = IndexSequenceStatePrivate::ItemId(Item);
            FFolder MovedFolder;
            if (!IndexSequenceStatePrivate::MoveFolderToGroup(Item.Folder, TargetGroup, MovedFolder))
            {
                return false;
            }
            Item = FIndexSequenceItem::FromFolder(MovedFolder);
            const FString NewId = IndexSequenceStatePrivate::ItemId(Item);
            if (OldId != NewId)
            {
                IndexSequenceStatePrivate::ReplaceIdEverywhere(State, OldId, NewId);
                MovedIds[MoveIndex] = NewId;
            }
        }
    }

    TArray<FString>& Destination = State.ChildrenByParent.FindOrAdd(TargetParentId);
    for (const FString& Id : MovedIds)
    {
        Destination.Remove(Id);
    }

    if (bAtTop)
    {
        for (int32 Index = MovedIds.Num() - 1; Index >= 0; --Index)
        {
            Destination.Insert(MovedIds[Index], 0);
        }
    }
    else
    {
        Destination.Append(MovedIds);
    }

    return IndexSequenceStatePrivate::Save(Level, State);
}

bool FIndexSequenceState::MoveItemsToParentAtTop(
    const TArray<FIndexSequenceItem>& MovedItems,
    const FIndexSequenceItem& ParentItem)
{
    if (!ParentItem.IsValid())
    {
        return false;
    }

    FIndexSequenceGroupKey TargetGroup;
    if (ParentItem.IsActor())
    {
        AActor* Parent = ParentItem.Actor.Get();
        if (!IsValid(Parent))
        {
            return false;
        }
        TargetGroup.Level = Parent->GetLevel();
        TargetGroup.bAttachedActorTier = true;
        TargetGroup.ParentActor = Parent;
    }
    else
    {
        TargetGroup.Level = ParentItem.Folder.GetRootObjectAssociatedLevel();
        TargetGroup.RootObject = ParentItem.Folder.GetRootObject();
        TargetGroup.FolderPath = ParentItem.Folder.GetPath();
        TargetGroup.ParentFolderGuid = ParentItem.Folder.GetActorFolderGuid();
    }

    return MoveItemsToGroupImpl(MovedItems, TargetGroup, true);
}

bool FIndexSequenceState::MoveItemsToRootAtBottom(
    const TArray<FIndexSequenceItem>& MovedItems,
    ULevel* Level,
    const FFolder::FRootObject& RootObject)
{
    if (!Level)
    {
        return false;
    }

    FIndexSequenceGroupKey TargetGroup;
    TargetGroup.Level = Level;
    TargetGroup.RootObject = RootObject;
    TargetGroup.FolderPath = FFolder::GetEmptyPath();
    return MoveItemsToGroupImpl(MovedItems, TargetGroup, false);
}

bool FIndexSequenceState::IntegrateItemsAtTop(const TArray<FIndexSequenceItem>& InItems)
{
    TArray<FIndexSequenceItem> Items = InItems;
    CollapseToTopLevelRoots(Items);
    SortItemsForMove(Items);
    bool bChanged = false;

    TMap<ULevel*, TArray<FIndexSequenceItem>> ByLevel;
    for (const FIndexSequenceItem& Item : Items)
    {
        if (ULevel* Level = IndexSequenceStatePrivate::GetLevel(Item))
        {
            ByLevel.FindOrAdd(Level).Add(Item);
        }
    }

    for (TPair<ULevel*, TArray<FIndexSequenceItem>>& LevelPair : ByLevel)
    {
        ULevel* Level = LevelPair.Key;
        IndexSequenceStatePrivate::FCachedLevelState& State = IndexSequenceStatePrivate::Load(Level);

        TMap<FString, TArray<FString>> ItemsByParent;
        for (const FIndexSequenceItem& Item : LevelPair.Value)
        {
            const FString Id = IndexSequenceStatePrivate::ItemId(Item);
            const FString Parent = IndexSequenceStatePrivate::ParentId(Item);
            if (Id.IsEmpty() || Parent.IsEmpty())
            {
                continue;
            }

            // If this stable row is already recorded beneath its current parent,
            // nothing structural changed. This is the key distinction that makes
            // actor/folder rename path changes order-neutral.
            const TArray<FString>* CurrentChildren = State.ChildrenByParent.Find(Parent);
            if (CurrentChildren && CurrentChildren->Contains(Id))
            {
                continue;
            }

            IndexSequenceStatePrivate::RemoveIdEverywhere(State, Id);
            ItemsByParent.FindOrAdd(Parent).Add(Id);
        }

        for (TPair<FString, TArray<FString>>& ParentPair : ItemsByParent)
        {
            TArray<FString>& Children = State.ChildrenByParent.FindOrAdd(ParentPair.Key);
            for (int32 Index = ParentPair.Value.Num() - 1; Index >= 0; --Index)
            {
                Children.Insert(ParentPair.Value[Index], 0);
            }
        }

        if (!ItemsByParent.IsEmpty())
        {
            bChanged |= IndexSequenceStatePrivate::Save(Level, State);
        }
    }

    return bChanged;
}

bool FIndexSequenceState::RemoveItem(const FIndexSequenceItem& Item)
{
    if (!Item.IsValid())
    {
        return false;
    }
    ULevel* Level = IndexSequenceStatePrivate::GetLevel(Item);
    if (!Level)
    {
        return false;
    }
    IndexSequenceStatePrivate::FCachedLevelState& State = IndexSequenceStatePrivate::Load(Level);
    const FString Id = IndexSequenceStatePrivate::ItemId(Item);
    IndexSequenceStatePrivate::RemoveIdEverywhere(State, Id);
    State.ChildrenByParent.Remove(Id);
    return IndexSequenceStatePrivate::Save(Level, State);
}

void FIndexSequenceState::CaptureNativeHierarchyUndoBaseline(ULevel* Level)
{
    IndexSequenceStatePrivate::CaptureNativeHierarchyUndoBaseline(Level);
}

bool FIndexSequenceState::HandleFolderMoved(const FFolder& OldFolder, const FFolder& NewFolder)
{
    if (!OldFolder.IsValid() || !NewFolder.IsValid())
    {
        return false;
    }
    ULevel* Level = NewFolder.GetRootObjectAssociatedLevel();
    if (!Level)
    {
        return false;
    }

    FIndexSequenceItem OldItem = FIndexSequenceItem::FromFolder(OldFolder);
    FIndexSequenceItem NewItem = FIndexSequenceItem::FromFolder(NewFolder);
    IndexSequenceStatePrivate::FCachedLevelState& State = IndexSequenceStatePrivate::Load(Level);
    FString OldId = IndexSequenceStatePrivate::ItemId(OldItem);
    const FString NewId = IndexSequenceStatePrivate::ItemId(NewItem);

    // Some folder implementations preserve their GUID across rename/move; some editor
    // paths can rebuild the folder object. Either way, preserve the row identity in our
    // tree by replacing the old identifier everywhere when necessary.
    if (OldId != NewId && !OldId.IsEmpty() && !NewId.IsEmpty())
    {
        IndexSequenceStatePrivate::ReplaceIdEverywhere(State, OldId, NewId);
        IndexSequenceStatePrivate::RemapNativeMoveBaselineId(Level, OldId, NewId);
        OldId = NewId;
    }

    const FString OldParent = IndexSequenceStatePrivate::ParentId(OldItem);
    const FString NewParent = IndexSequenceStatePrivate::ParentId(NewItem);
    if (OldParent == NewParent)
    {
        // Rename only. The row stays exactly where it was.
        return IndexSequenceStatePrivate::Save(Level, State);
    }

    // Native hierarchy move. Leave the row recorded at its previous location until
    // the destination sibling tier is sorted. EnsureCaptured() can then recover the
    // row's pre-move visual path and reconcile an entire native multi-item move as one
    // ordered block. Eagerly inserting every folder at index 0 here reverses folder
    // order during Create Folder containing selection. Explicit Index edge moves are
    // handled elsewhere under the explicit-reorder guard.
    return IndexSequenceStatePrivate::Save(Level, State);
}

FIndexSequenceItem FIndexSequenceState::ResolveCurrentItem(const FIndexSequenceItem& Item)
{
    if (!Item.IsValid())
    {
        return FIndexSequenceItem();
    }
    if (Item.IsActor())
    {
        return Item.Actor.IsValid() ? Item : FIndexSequenceItem();
    }
    ULevel* Level = Item.Folder.GetRootObjectAssociatedLevel();
    if (!Level)
    {
        return FIndexSequenceItem();
    }
    const FString Id = IndexSequenceStatePrivate::ItemId(Item);
    FFolder Current;
    if (IndexSequenceStatePrivate::ResolveFolderById(Level, Id, Current))
    {
        return FIndexSequenceItem::FromFolder(Current);
    }
    return Item.Folder.IsValid() ? Item : FIndexSequenceItem();
}

bool FIndexSequenceState::IsApplyingExplicitReorder()
{
    return IndexSequenceStatePrivate::ExplicitReorderDepth > 0;
}

void FIndexSequenceState::PrepareForPostUndoRedo()
{
    // Preserve NativeMoveBaselines here because they carry the session-local parent
    // slot history needed to reconstruct rows returning during Undo/Redo. Only the
    // persisted-state cache is invalidated; map changes and full resets still call
    // InvalidateCache(), which clears everything.
    IndexSequenceStatePrivate::BeginPostUndoRedoRestoreWindow();
    IndexSequenceStatePrivate::Cache.Reset();
}

void FIndexSequenceState::InvalidateCache()
{
    IndexSequenceStatePrivate::Cache.Reset();
    IndexSequenceStatePrivate::NativeMoveBaselines.Reset();
    IndexSequenceStatePrivate::PendingUndoRestoreStates.Reset();
    IndexSequenceStatePrivate::PostUndoRedoRestoreUntilFrame = 0;
}

#undef LOCTEXT_NAMESPACE
