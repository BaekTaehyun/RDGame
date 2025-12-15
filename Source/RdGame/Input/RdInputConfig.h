#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RdInputConfig.generated.h"

class UInputAction;

/**
 * FRdInputAction
 *
 * Struct used to map an input action to a gameplay input tag.
 */
USTRUCT(BlueprintType)
struct FRdInputAction {
  GENERATED_BODY()

public:
  /** The input action to bind */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
  TObjectPtr<const UInputAction> InputAction = nullptr;

  /** The gameplay tag associated with this input action */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (Categories = "InputTag"))
  FGameplayTag InputTag;
};

/**
 * URdInputConfig
 *
 * Non-mutable data asset that contains input configuration properties.
 * Maps InputActions to GameplayTags for a unified input system.
 *
 * Usage:
 * 1. Create a DataAsset of this type in the editor (e.g., DA_RdInputConfig)
 * 2. Add entries to NativeInputActions and/or AbilityInputActions
 * 3. Use FindNativeInputActionForTag() or FindAbilityInputActionForTag() to
 * look up InputActions
 */
UCLASS(BlueprintType, Const)
class RDGAME_API URdInputConfig : public UDataAsset {
  GENERATED_BODY()

public:
  URdInputConfig(const FObjectInitializer &ObjectInitializer);

  /** Find a native input action by its associated gameplay tag */
  UFUNCTION(BlueprintCallable, Category = "RdGame|Input")
  const UInputAction *
  FindNativeInputActionForTag(const FGameplayTag &InputTag,
                              bool bLogNotFound = true) const;

  /** Find an ability input action by its associated gameplay tag */
  UFUNCTION(BlueprintCallable, Category = "RdGame|Input")
  const UInputAction *
  FindAbilityInputActionForTag(const FGameplayTag &InputTag,
                               bool bLogNotFound = true) const;

public:
  /**
   * List of native input actions used by the owner.
   * These input actions are mapped to a gameplay tag and must be manually
   * bound. Examples: Move, Look, Jump, Crouch
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (TitleProperty = "InputAction"))
  TArray<FRdInputAction> NativeInputActions;

  /**
   * List of ability input actions used by the owner.
   * These input actions are mapped to a gameplay tag and are automatically
   * bound to abilities with matching input tags. Examples: PrimaryAttack,
   * SecondaryAttack, Ability1, Ability2
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (TitleProperty = "InputAction"))
  TArray<FRdInputAction> AbilityInputActions;
};
