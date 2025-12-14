#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "RdHUD.generated.h"


class UCommonActivatableWidget;
class UObject;
struct FFrame;

/**
 * ARdHUD
 *
 * Base HUD class for RDGame.
 * Acts as an anchor for UI systems and implements the GameFeature init state
 * interface to allow UI extensions to wait for the HUD to be ready.
 */
UCLASS(Config = Game)
class RDGAME_API ARdHUD : public AHUD, public IGameFrameworkInitStateInterface {
  GENERATED_BODY()

public:
  ARdHUD(
      const FObjectInitializer &ObjectInitializer = FObjectInitializer::Get());

  //~AActor interface
  virtual void PreInitializeComponents() override;
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;
  //~End of AActor interface

  //~IGameFrameworkInitStateInterface interface
  virtual FName GetFeatureName() const override;
  virtual bool CanChangeInitState(UGameFrameworkComponentManager *Manager,
                                  FGameplayTag CurrentState,
                                  FGameplayTag DesiredState) const override;
  virtual void HandleChangeInitState(UGameFrameworkComponentManager *Manager,
                                     FGameplayTag CurrentState,
                                     FGameplayTag DesiredState) override;
  virtual void
  OnActorInitStateChanged(const FActorInitStateChangedParams &Params) override;
  virtual void CheckDefaultInitialization() override;
  //~End of IGameFrameworkInitStateInterface interface

protected:
  // Handles creating the UI layout
  void CreateUI();

private:
  static const FName HUD_InitState_UIReady;
};
