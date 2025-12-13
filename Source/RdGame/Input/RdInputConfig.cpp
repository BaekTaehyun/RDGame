#include "Input/RdInputConfig.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RdInputConfig)

URdInputConfig::URdInputConfig(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}

const UInputAction *
URdInputConfig::FindNativeInputActionForTag(const FGameplayTag &InputTag,
                                            bool bLogNotFound) const {
  for (const FRdInputAction &Action : NativeInputActions) {
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
URdInputConfig::FindAbilityInputActionForTag(const FGameplayTag &InputTag,
                                             bool bLogNotFound) const {
  for (const FRdInputAction &Action : AbilityInputActions) {
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
