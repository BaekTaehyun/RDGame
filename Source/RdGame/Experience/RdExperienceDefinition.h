#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RdExperienceDefinition.generated.h"


/**
 * URdExperienceDefinition
 *
 * Defines a specific "Game Experience" (Location + Context).
 * Replaces hardcoded Level loading/Network connections.
 */
UCLASS(BlueprintType, Const)
class RDGAME_API URdExperienceDefinition : public UPrimaryDataAsset {
  GENERATED_BODY()

public:
  // The unique ID for this experience (e.g., Exp.Dungeon.Fire)
  // Used by the Transfer Volume to identify where to go.
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Experience")
  FGameplayTag ExperienceId;

  // The Server IP to connect to (if applicable)
  // "127.0.0.1" for local server, or use backend to overwrite this.
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network")
  FString TargetIP = "127.0.0.1";

  // The UDP Port (e.g., 7777)
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Network")
  int32 TargetPort = 7777;

  // The map to load (Reference mainly for dedicated server or local play
  // testing) The client will actually travel to URL built from IP:Port.
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "Map",
            meta = (AllowedTypes = "World"))
  FSoftObjectPath TargetMap;

  // Loading Screen Widget to display during transition
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
  TSubclassOf<UUserWidget> LoadingScreenWidget;

  // Optional: Help text or Title to show on loading screen
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly, Category = "UI")
  FText ExperienceTitle;

  // Helper to build the connection URL
  FString GetConnectionURL() const {
    return FString::Printf(TEXT("%s:%d"), *TargetIP, TargetPort);
  }

  // Primary Data Asset ID
  virtual FPrimaryAssetId GetPrimaryAssetId() const override {
    return FPrimaryAssetId(TEXT("RdExperienceDefinition"), GetFName());
  }
};
