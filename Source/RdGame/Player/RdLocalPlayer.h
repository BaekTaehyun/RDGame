// Copyright RDGame. All Rights Reserved.

#pragma once

#include "CommonLocalPlayer.h"
#include "CoreMinimal.h"
#include "RdLocalPlayer.generated.h"

class APlayerController;

/**
 * URdLocalPlayer
 *
 * Custom LocalPlayer class for RDGame.
 * Inherits from UCommonLocalPlayer to enable UI Policy integration.
 *
 * Future extensions:
 * - Team system integration
 * - Local/Shared settings management
 * - Audio device management
 */
UCLASS()
class RDGAME_API URdLocalPlayer : public UCommonLocalPlayer {
  GENERATED_BODY()

public:
  URdLocalPlayer();

  //~ULocalPlayer interface
  virtual bool SpawnPlayActor(const FString &URL, FString &OutError,
                              UWorld *InWorld) override;
  //~End of ULocalPlayer interface

protected:
  /** Called when the player controller changes */
  virtual void OnPlayerControllerChanged(APlayerController *NewController);
};
