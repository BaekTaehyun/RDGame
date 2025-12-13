// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"

#include "ReBirthInputConfig.generated.h"

class UInputAction;

/**
 * FReBirthInputAction
 *
 * Struct used to map an input action to a gameplay input tag.
 */
USTRUCT(BlueprintType)
struct FReBirthInputAction {
  GENERATED_BODY()

public:
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
  TObjectPtr<const UInputAction> InputAction = nullptr;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (Categories = "InputTag"))
  FGameplayTag InputTag;
};

/**
 * UReBirthInputConfig
 *
 * Non-mutable data asset that contains input configuration properties.
 * Maps GameplayTags to InputActions for flexible input binding.
 */
UCLASS(BlueprintType, Const)
class REBIRTH_API UReBirthInputConfig : public UDataAsset {
  GENERATED_BODY()

public:
  UReBirthInputConfig(const FObjectInitializer &ObjectInitializer);

  /** Find a native input action by its gameplay tag */
  UFUNCTION(BlueprintCallable, Category = "ReBirth|Input")
  const UInputAction *
  FindNativeInputActionForTag(const FGameplayTag &InputTag,
                              bool bLogNotFound = true) const;

  /** Find an ability input action by its gameplay tag */
  UFUNCTION(BlueprintCallable, Category = "ReBirth|Input")
  const UInputAction *
  FindAbilityInputActionForTag(const FGameplayTag &InputTag,
                               bool bLogNotFound = true) const;

public:
  /**
   * List of input actions used for native gameplay (movement, camera, etc.).
   * These input actions are mapped to a gameplay tag and must be manually
   * bound.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (TitleProperty = "InputAction"))
  TArray<FReBirthInputAction> NativeInputActions;

  /**
   * List of input actions used for abilities.
   * These input actions are mapped to a gameplay tag and are automatically
   * bound to abilities with matching input tags.
   */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (TitleProperty = "InputAction"))
  TArray<FReBirthInputAction> AbilityInputActions;
};
