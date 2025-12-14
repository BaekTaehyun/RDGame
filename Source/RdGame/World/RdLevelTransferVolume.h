#pragma once

#include "Components/ArrowComponent.h"
#include "Components/BoxComponent.h"
#include "CoreMinimal.h"
#include "Experience/RdExperienceDefinition.h"
#include "GameFramework/Actor.h"
#include "GameplayTagContainer.h"
#include "RdLevelTransferVolume.generated.h"


/**
 * ARdLevelTransferVolume
 *
 * Trigger volume that initiates an Experience Transition (Session Join).
 */
UCLASS()
class RDGAME_API ARdLevelTransferVolume : public AActor {
  GENERATED_BODY()

public:
  ARdLevelTransferVolume();

protected:
  virtual void BeginPlay() override;

public:
  // The Experience to trigger (e.g., Exp.Dungeon.Fire)
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Experience")
  FGameplayTag DestinationTag;

  // Optional: Direct reference to force load the asset
  // If set, this ensures the asset is loaded in memory for the subsystem to
  // find.
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Experience")
  class URdExperienceDefinition *DestinationAsset;

  // Trigger Component
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
  UBoxComponent *TransferTrigger;

  // Visual helper
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components")
  UArrowComponent *Arrow;

  UFUNCTION()
  void OnOverlapBegin(UPrimitiveComponent *OverlappedComp, AActor *OtherActor,
                      UPrimitiveComponent *OtherComp, int32 OtherBodyIndex,
                      bool bFromSweep, const FHitResult &SweepResult);
};
