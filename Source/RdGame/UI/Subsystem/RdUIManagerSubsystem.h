// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "GameUIManagerSubsystem.h"
#include "RdUIManagerSubsystem.generated.h"

class FSubsystemCollectionBase;
class UObject;

/**
 * URdUIManagerSubsystem
 *
 * Manages the global UI state and coordinates with the gameplay layer.
 * Handles HUD visibility synchronization.
 */
UCLASS()
class RDGAME_API URdUIManagerSubsystem : public UGameUIManagerSubsystem
{
	GENERATED_BODY()

public:
	URdUIManagerSubsystem();

	//~UGameInstanceSubsystem interface
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	//~End of UGameInstanceSubsystem interface

private:
	bool Tick(float DeltaTime);
	void SyncRootLayoutVisibilityToShowHUD();
	
	FTSTicker::FDelegateHandle TickHandle;
};
