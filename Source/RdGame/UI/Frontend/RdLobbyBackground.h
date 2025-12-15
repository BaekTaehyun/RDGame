// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"

#include "RdLobbyBackground.generated.h"

class UObject;
class UWorld;

/**
 * URdLobbyBackground
 *
 * Data asset for configuring the lobby/frontend background level.
 */
UCLASS(config=EditorPerProjectUserSettings)
class RDGAME_API URdLobbyBackground : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Background")
	TSoftObjectPtr<UWorld> BackgroundLevel;
};
