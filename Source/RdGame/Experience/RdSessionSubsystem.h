#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "RdExperienceDefinition.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RdSessionSubsystem.generated.h"



DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnTravelRequestStarted,
                                            const URdExperienceDefinition *,
                                            ExperienceDef);
DECLARE_DYNAMIC_MULTICAST_DELEGATE(FOnTravelFinished);

/**
 * URdSessionSubsystem
 *
 * Manages high-level session transitions (Experiences).
 * Handles the "Lifecycle" of moving from Field -> Dungeon.
 */
UCLASS()
class RDGAME_API URdSessionSubsystem : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;

  // Helper to get subsystem from Blueprint
  UFUNCTION(BlueprintPure, Category = "RdSession",
            meta = (WorldContext = "WorldContextObject"))
  static URdSessionSubsystem *Get(const UObject *WorldContextObject);

  /**
   * Request to join a specific experience.
   * Starts the transition process: UI Fade -> Save Data -> ClientTravel
   */
  UFUNCTION(BlueprintCallable, Category = "RdSession")
  void RequestExperience(FGameplayTag ExperienceTag);

  // Event hooks for UI
  UPROPERTY(BlueprintAssignable, Category = "RdSession")
  FOnTravelRequestStarted OnTravelRequestStarted;

  UPROPERTY(BlueprintAssignable, Category = "RdSession")
  FOnTravelFinished OnTravelFinished;

private:
  // Internal helper to perform the actual travel
  void ExecuteTravel(const URdExperienceDefinition *ExpDef);

  // Find the asset from the AssetManager (or internal registry)
  const URdExperienceDefinition *FindExperienceDefinition(FGameplayTag Tag);

  // Cache of loaded definitions (Simple version: In a real project, use
  // AssetManager)
  UPROPERTY()
  TArray<URdExperienceDefinition *> LoadedExperiences;
};
