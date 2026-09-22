#pragma once

#include "CoreMinimal.h"
#include "Framework/Application/IInputProcessor.h"
#include "IndexSequenceState.h"

class ISceneOutliner;
class UWorld;
struct FFolder;

namespace IndexInteraction
{
    void RegisterOutliner(ISceneOutliner& Outliner);
    void UnregisterDeadOutliners();
    void RefreshAllOutliners();
    void HandleNativeHierarchyChanged(AActor* Actor);
    void HandleActorDeleted(AActor* Actor);
    void HandleFolderMoved(UWorld& World, const FFolder& OldFolder, const FFolder& NewFolder);
    void FlashRenameItem(const FIndexSequenceItem& Item);
    void ResetTransientState();
    bool IntegrateNewActorsAtTop(const TArray<AActor*>& Actors);
    bool GetSelectedSequenceRoots(TArray<FIndexSequenceItem>& OutItems);
    TSharedRef<IInputProcessor> CreateInputProcessor();
    void Shutdown();
}
