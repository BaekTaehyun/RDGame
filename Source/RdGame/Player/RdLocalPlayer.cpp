// Copyright RDGame. All Rights Reserved.

#include "Player/RdLocalPlayer.h"

#include "Engine/World.h"
#include "GameFramework/PlayerController.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RdLocalPlayer)

URdLocalPlayer::URdLocalPlayer() {}

bool URdLocalPlayer::SpawnPlayActor(const FString &URL, FString &OutError,
                                    UWorld *InWorld) {
  const bool bResult = Super::SpawnPlayActor(URL, OutError, InWorld);

  OnPlayerControllerChanged(PlayerController);

  return bResult;
}

void URdLocalPlayer::OnPlayerControllerChanged(
    APlayerController *NewController) {
  // Future: Add team system integration here
  // Future: Add settings loading here

  UE_LOG(LogTemp, Log,
         TEXT("URdLocalPlayer::OnPlayerControllerChanged - Controller: %s"),
         NewController ? *NewController->GetName() : TEXT("None"));
}
