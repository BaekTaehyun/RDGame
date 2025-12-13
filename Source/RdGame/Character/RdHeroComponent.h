#pragma once

#include "Components/GameFrameworkInitStateInterface.h"
#include "Components/PawnComponent.h"
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "InputMappingContext.h"
#include "RdHeroComponent.generated.h"

class UGameFrameworkComponentManager;
class UInputComponent;
class URdInputConfig;
struct FActorInitStateChangedParams;
struct FGameplayTag;
struct FInputActionValue;

/**
 * Input Mapping Context and Priority pair for configuration
 */
USTRUCT(BlueprintType)
struct FRdInputMappingContextAndPriority {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  TSoftObjectPtr<UInputMappingContext> InputMapping;

  // Higher priority input mappings will be prioritized over mappings with a
  // lower priority.
  UPROPERTY(EditAnywhere, BlueprintReadOnly)
  int32 Priority = 0;
};

/**
 * URdHeroComponent
 *
 * Component that sets up input handling for player controlled pawns.
 * Uses the ModularGameplay plugin's InitState system to coordinate
 * initialization.
 */
UCLASS(Blueprintable, Meta = (BlueprintSpawnableComponent))
class RDGAME_API URdHeroComponent : public UPawnComponent,
                                    public IGameFrameworkInitStateInterface {
  GENERATED_BODY()

public:
  URdHeroComponent(const FObjectInitializer &ObjectInitializer);

  /** Returns the hero component if one exists on the specified actor. */
  UFUNCTION(BlueprintPure, Category = "RdGame|Hero")
  static URdHeroComponent *FindHeroComponent(const AActor *Actor) {
    return (Actor ? Actor->FindComponentByClass<URdHeroComponent>() : nullptr);
  }

  /** Returns the input config data asset */
  const URdInputConfig *GetInputConfig() const { return InputConfig; }

  /** Type-safe method to get the owner pawn's controller */
  template <class T> T *GetController() const {
    if (APawn *Pawn = GetPawn<APawn>()) {
      return Cast<T>(Pawn->GetController());
    }
    return nullptr;
  }

  /** True if this is controlled by a real player and has progressed far enough
   * in initialization where additional input bindings can be added */
  bool IsReadyToBindInputs() const;

  /** The name of the extension event sent via UGameFrameworkComponentManager
   * when ability inputs are ready to bind */
  static const FName NAME_BindInputsNow;

  /** The name of this component-implemented feature */
  static const FName NAME_ActorFeatureName;

  //~ Begin IGameFrameworkInitStateInterface interface
  virtual FName GetFeatureName() const override {
    return NAME_ActorFeatureName;
  }
  virtual bool CanChangeInitState(UGameFrameworkComponentManager *Manager,
                                  FGameplayTag CurrentState,
                                  FGameplayTag DesiredState) const override;
  virtual void HandleChangeInitState(UGameFrameworkComponentManager *Manager,
                                     FGameplayTag CurrentState,
                                     FGameplayTag DesiredState) override;
  virtual void
  OnActorInitStateChanged(const FActorInitStateChangedParams &Params) override;
  virtual void CheckDefaultInitialization() override;
  //~ End IGameFrameworkInitStateInterface interface

protected:
  virtual void OnRegister() override;
  virtual void BeginPlay() override;
  virtual void EndPlay(const EEndPlayReason::Type EndPlayReason) override;

  virtual void InitializePlayerInput(UInputComponent *PlayerInputComponent);

  void Input_AbilityInputTagPressed(FGameplayTag InputTag);
  void Input_AbilityInputTagReleased(FGameplayTag InputTag);

  void Input_Move(const FInputActionValue &InputActionValue);
  void Input_LookMouse(const FInputActionValue &InputActionValue);
  void Input_LookStick(const FInputActionValue &InputActionValue);
  void Input_Jump(const FInputActionValue &InputActionValue);
  void Input_Crouch(const FInputActionValue &InputActionValue);

protected:
  /** Default input mappings to apply when this component initializes */
  UPROPERTY(EditAnywhere, Category = "Input")
  TArray<FRdInputMappingContextAndPriority> DefaultInputMappings;

  /** The input config data asset that maps GameplayTags to InputActions */
  UPROPERTY(EditAnywhere, Category = "Input")
  TObjectPtr<URdInputConfig> InputConfig;

  /** True when player input bindings have been applied, will never be true for
   * non-players */
  bool bReadyToBindInputs;
};
