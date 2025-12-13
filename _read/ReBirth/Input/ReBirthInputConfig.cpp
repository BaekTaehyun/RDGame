// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ReBirthInputConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReBirthInputConfig)

UReBirthInputConfig::UReBirthInputConfig(
    const FObjectInitializer &ObjectInitializer) {}

const UInputAction *
UReBirthInputConfig::FindNativeInputActionForTag(const FGameplayTag &InputTag,
                                                 bool bLogNotFound) const {
  for (const FReBirthInputAction &Action : NativeInputActions) {
    if (Action.InputAction && (Action.InputTag == InputTag)) {
      return Action.InputAction;
    }
  }

  if (bLogNotFound) {
    UE_LOG(LogTemp, Error,
           TEXT("Can't find NativeInputAction for InputTag [%s] on InputConfig "
                "[%s]."),
           *InputTag.ToString(), *GetNameSafe(this));
  }

  return nullptr;
}

const UInputAction *
UReBirthInputConfig::FindAbilityInputActionForTag(const FGameplayTag &InputTag,
                                                  bool bLogNotFound) const {
  for (const FReBirthInputAction &Action : AbilityInputActions) {
    if (Action.InputAction && (Action.InputTag == InputTag)) {
      return Action.InputAction;
    }
  }

  if (bLogNotFound) {
    UE_LOG(LogTemp, Error,
           TEXT("Can't find AbilityInputAction for InputTag [%s] on "
                "InputConfig [%s]."),
           *InputTag.ToString(), *GetNameSafe(this));
  }

  return nullptr;
}
