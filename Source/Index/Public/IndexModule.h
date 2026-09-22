// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Modules/ModuleManager.h"

class IInputProcessor;
class AActor;

class FIndexModule : public IModuleInterface
{
public:
    virtual void StartupModule() override;
    virtual void ShutdownModule() override;
    virtual bool SupportsDynamicReloading() override { return false; }

private:
    FDelegateHandle ActorBrowserColumnsHandle;
    FDelegateHandle PostUndoRedoHandle;
    FDelegateHandle ObjectsReplacedHandle;
    FDelegateHandle NewActorsPlacedHandle;
    FDelegateHandle LevelActorAttachedHandle;
    FDelegateHandle LevelActorDetachedHandle;
    FDelegateHandle LevelActorDeletedHandle;
    FDelegateHandle MapChangeHandle;
    FDelegateHandle DuplicateActorsBeginHandle;
    FDelegateHandle DuplicateActorsEndHandle;
    FDelegateHandle FolderMovedHandle;
    FDelegateHandle FolderDeletedHandle;
    bool bDuplicateInProgress = false;
    TSharedPtr<IInputProcessor> InputProcessor;
};
