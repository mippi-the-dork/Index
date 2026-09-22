#include "Misc/AutomationTest.h"
#include "IndexSequenceState.h"
#include "Engine/World.h"
#include "GameFramework/Actor.h"

BEGIN_DEFINE_SPEC(FIndexHierarchyStateSpec, "Index.HierarchyState", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
UWorld* World = nullptr;
END_DEFINE_SPEC(FIndexHierarchyStateSpec)

void FIndexHierarchyStateSpec::Define()
{
    BeforeEach([this]()
    {
        World = UWorld::CreateWorld(EWorldType::Editor, false);
        FIndexSequenceState::InvalidateCache();
    });

    AfterEach([this]()
    {
        FIndexSequenceState::InvalidateCache();
        if (World)
        {
            World->DestroyWorld(false);
            World = nullptr;
        }
    });

    It("captures the current sibling order", [this]()
    {
        AActor* A = World->SpawnActor<AActor>();
        AActor* B = World->SpawnActor<AActor>();
        AActor* C = World->SpawnActor<AActor>();
        TArray<FIndexSequenceItem> Items = {
            FIndexSequenceItem::FromActor(A),
            FIndexSequenceItem::FromActor(B),
            FIndexSequenceItem::FromActor(C)
        };
        FIndexSequenceState::EnsureCaptured(Items);
        TestEqual("A is first", FIndexSequenceState::GetOrder(Items[0]), 0);
        TestEqual("B is second", FIndexSequenceState::GetOrder(Items[1]), 1);
        TestEqual("C is third", FIndexSequenceState::GetOrder(Items[2]), 2);
    });

    It("moves a block before a target while preserving source order", [this]()
    {
        AActor* A = World->SpawnActor<AActor>();
        AActor* B = World->SpawnActor<AActor>();
        AActor* C = World->SpawnActor<AActor>();
        AActor* D = World->SpawnActor<AActor>();
        TArray<FIndexSequenceItem> Initial = {
            FIndexSequenceItem::FromActor(A), FIndexSequenceItem::FromActor(B),
            FIndexSequenceItem::FromActor(C), FIndexSequenceItem::FromActor(D)
        };
        FIndexSequenceState::EnsureCaptured(Initial);

        TestTrue("Move succeeds", FIndexSequenceState::InsertItemsRelative(
            { FIndexSequenceItem::FromActor(C), FIndexSequenceItem::FromActor(B) },
            FIndexSequenceItem::FromActor(A), true));

        TArray<FIndexSequenceItem> Result = Initial;
        FIndexSequenceState::SortItems(Result);
        TestTrue("B first", Result[0].Actor.Get() == B);
        TestTrue("C second", Result[1].Actor.Get() == C);
        TestTrue("A third", Result[2].Actor.Get() == A);
        TestTrue("D fourth", Result[3].Actor.Get() == D);
    });

    It("places new content at the top of its local tier", [this]()
    {
        AActor* ExistingA = World->SpawnActor<AActor>();
        AActor* ExistingB = World->SpawnActor<AActor>();
        AActor* NewActor = World->SpawnActor<AActor>();
        FIndexSequenceState::EnsureCaptured({
            FIndexSequenceItem::FromActor(ExistingA),
            FIndexSequenceItem::FromActor(ExistingB)
        });
        TestTrue("Integration succeeds", FIndexSequenceState::IntegrateItemsAtTop({
            FIndexSequenceItem::FromActor(NewActor)
        }));
        TestEqual("New actor is first", FIndexSequenceState::GetOrder(FIndexSequenceItem::FromActor(NewActor)), 0);
    });

    It("preserves source order when native actor moves arrive incrementally", [this]()
    {
        AActor* A = World->SpawnActor<AActor>();
        AActor* B = World->SpawnActor<AActor>();
        A->SetFolderPath_Recursively(FName(TEXT("Source")));
        B->SetFolderPath_Recursively(FName(TEXT("Source")));

        FIndexSequenceState::EnsureCaptured({
            FIndexSequenceItem::FromActor(A),
            FIndexSequenceItem::FromActor(B)
        });

        // Simulate Unreal moving selected actors one at a time and re-sorting the
        // destination between moves. The second native snapshot is deliberately
        // reversed to prove Index uses the immutable pre-move visual order.
        A->SetFolderPath_Recursively(FName(TEXT("Destination")));
        FIndexSequenceState::EnsureCaptured({ FIndexSequenceItem::FromActor(A) });

        B->SetFolderPath_Recursively(FName(TEXT("Destination")));
        FIndexSequenceState::EnsureCaptured({
            FIndexSequenceItem::FromActor(B),
            FIndexSequenceItem::FromActor(A)
        });

        TArray<FIndexSequenceItem> Destination = {
            FIndexSequenceItem::FromActor(B),
            FIndexSequenceItem::FromActor(A)
        };
        FIndexSequenceState::SortItems(Destination);

        TestTrue("A remains first", Destination[0].Actor.Get() == A);
        TestTrue("B remains second", Destination[1].Actor.Get() == B);
    });

    It("restores a returning actor to its historical sibling slot", [this]()
    {
        AActor* A = World->SpawnActor<AActor>();
        AActor* B = World->SpawnActor<AActor>();
        A->SetFolderPath_Recursively(FName(TEXT("Source")));
        B->SetFolderPath_Recursively(FName(TEXT("Source")));

        FIndexSequenceState::EnsureCaptured({
            FIndexSequenceItem::FromActor(A),
            FIndexSequenceItem::FromActor(B)
        });

        // Simulate the forward half of a mixed native hierarchy operation: B leaves
        // Source and Index consumes its old canonical slot when the destination tier
        // is reconciled.
        B->SetFolderPath_Recursively(FName(TEXT("Destination")));
        FIndexSequenceState::EnsureCaptured({ FIndexSequenceItem::FromActor(B) });

        // Now simulate Undo returning B to Source while Unreal presents the native
        // row order as B,A. B historically belonged after A, so reconciliation must
        // merge it with the local tracked siblings instead of treating it as a new
        // incoming row at the front.
        B->SetFolderPath_Recursively(FName(TEXT("Source")));
        FIndexSequenceState::EnsureCaptured({
            FIndexSequenceItem::FromActor(B),
            FIndexSequenceItem::FromActor(A)
        });

        TArray<FIndexSequenceItem> Source = {
            FIndexSequenceItem::FromActor(B),
            FIndexSequenceItem::FromActor(A)
        };
        FIndexSequenceState::SortItems(Source);

        TestTrue("A is restored first", Source[0].Actor.Get() == A);
        TestTrue("B is restored second", Source[1].Actor.Get() == B);
    });

    It("keeps actor rename independent from hierarchy order", [this]()
    {
        AActor* A = World->SpawnActor<AActor>();
        AActor* B = World->SpawnActor<AActor>();
        TArray<FIndexSequenceItem> Items = {
            FIndexSequenceItem::FromActor(A), FIndexSequenceItem::FromActor(B)
        };
        FIndexSequenceState::EnsureCaptured(Items);
        const int32 Before = FIndexSequenceState::GetOrder(Items[1]);
        B->SetActorLabel(TEXT("AAA Renamed"));
        TestEqual("Rename does not change order", FIndexSequenceState::GetOrder(Items[1]), Before);
    });
}
