// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "EnhancedInputComponent.h"
#include "Input/ReBirthInputConfig.h"

#include "ReBirthInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;

/**
 * UReBirthInputComponent
 *
 * Component used to manage input mappings and bindings using an input config
 * data asset. Extends UEnhancedInputComponent with GameplayTag-based binding
 * support.
 */
UCLASS(Config = Input)
class REBIRTH_API UReBirthInputComponent : public UEnhancedInputComponent {
  GENERATED_BODY()

public:
  UReBirthInputComponent(const FObjectInitializer &ObjectInitializer);

  void
  AddInputMappings(const UReBirthInputConfig *InputConfig,
                   UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const;
  void
  RemoveInputMappings(const UReBirthInputConfig *InputConfig,
                      UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const;

  /**
   * Bind a native input action (movement, camera, etc.) to a function.
   * @param InputConfig - The input configuration asset
   * @param InputTag - The gameplay tag identifying the input action
   * @param TriggerEvent - When to trigger the binding (Started, Triggered,
   * Completed, etc.)
   * @param Object - The object that owns the function
   * @param Func - The function to call when the input is triggered
   * @param bLogIfNotFound - Whether to log an error if the action is not found
   */
  template <class UserClass, typename FuncType>
  void BindNativeAction(const UReBirthInputConfig *InputConfig,
                        const FGameplayTag &InputTag,
                        ETriggerEvent TriggerEvent, UserClass *Object,
                        FuncType Func, bool bLogIfNotFound);

  /**
   * Bind all ability input actions from the config to pressed/released
   * handlers.
   * @param InputConfig - The input configuration asset
   * @param Object - The object that owns the functions
   * @param PressedFunc - Function to call when ability input is pressed
   * @param ReleasedFunc - Function to call when ability input is released
   * @param BindHandles - Output array of bind handles for later removal
   */
  template <class UserClass, typename PressedFuncType,
            typename ReleasedFuncType>
  void BindAbilityActions(const UReBirthInputConfig *InputConfig,
                          UserClass *Object, PressedFuncType PressedFunc,
                          ReleasedFuncType ReleasedFunc,
                          TArray<uint32> &BindHandles);

  /** Remove bindings by their handles */
  void RemoveBinds(TArray<uint32> &BindHandles);
};

template <class UserClass, typename FuncType>
void UReBirthInputComponent::BindNativeAction(
    const UReBirthInputConfig *InputConfig, const FGameplayTag &InputTag,
    ETriggerEvent TriggerEvent, UserClass *Object, FuncType Func,
    bool bLogIfNotFound) {
  check(InputConfig);
  if (const UInputAction *IA =
          InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound)) {
    BindAction(IA, TriggerEvent, Object, Func);
  }
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void UReBirthInputComponent::BindAbilityActions(
    const UReBirthInputConfig *InputConfig, UserClass *Object,
    PressedFuncType PressedFunc, ReleasedFuncType ReleasedFunc,
    TArray<uint32> &BindHandles) {
  check(InputConfig);

  for (const FReBirthInputAction &Action : InputConfig->AbilityInputActions) {
    if (Action.InputAction && Action.InputTag.IsValid()) {
      if (PressedFunc) {
        BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Triggered,
                                   Object, PressedFunc, Action.InputTag)
                            .GetHandle());
      }

      if (ReleasedFunc) {
        BindHandles.Add(BindAction(Action.InputAction, ETriggerEvent::Completed,
                                   Object, ReleasedFunc, Action.InputTag)
                            .GetHandle());
      }
    }
  }
}
