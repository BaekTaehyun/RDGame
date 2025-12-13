#pragma once

#include "CoreMinimal.h"
#include "EnhancedInputComponent.h"
#include "Input/RdInputConfig.h"
#include "RdInputComponent.generated.h"

class UEnhancedInputLocalPlayerSubsystem;

/**
 * GameplayTag 기반 입력 바인딩을 지원하는 Enhanced Input Component
 */
UCLASS(ClassGroup = Input, meta = (BlueprintSpawnableComponent))
class RDGAME_API URdInputComponent : public UEnhancedInputComponent {
  GENERATED_BODY()

public:
  URdInputComponent(const FObjectInitializer &ObjectInitializer);

  void
  AddInputMappings(const URdInputConfig *InputConfig,
                   UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const;
  void
  RemoveInputMappings(const URdInputConfig *InputConfig,
                      UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const;

  /**
   * Native 입력 바인딩 (이동, 카메라 등)
   */
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
  void BindNativeAction(const URdInputConfig *InputConfig,
                        const FGameplayTag &InputTag,
                        ETriggerEvent TriggerEvent, UserClass *Object,
                        FuncType Func, bool bLogIfNotFound);

  /**
   * Ability 입력 일괄 바인딩 (공격, 스킬 등)
   */

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
  void BindAbilityActions(const URdInputConfig *InputConfig, UserClass *Object,
                          PressedFuncType PressedFunc,
                          ReleasedFuncType ReleasedFunc,
                          TArray<uint32> &BindHandles);

  /** 바인딩 핸들로 제거 */
  void RemoveBinds(TArray<uint32> &BindHandles);
};

// ---------------------------------------------------------
// Template Implementations
// ---------------------------------------------------------

template <class UserClass, typename FuncType>
void URdInputComponent::BindNativeAction(const URdInputConfig *InputConfig,
                                         const FGameplayTag &InputTag,
                                         ETriggerEvent TriggerEvent,
                                         UserClass *Object, FuncType Func,
                                         bool bLogIfNotFound) {
  check(InputConfig);
  if (const UInputAction *IA =
          InputConfig->FindNativeInputActionForTag(InputTag, bLogIfNotFound)) {
    BindAction(IA, TriggerEvent, Object, Func);
  }
}

template <class UserClass, typename PressedFuncType, typename ReleasedFuncType>
void URdInputComponent::BindAbilityActions(const URdInputConfig *InputConfig,
                                           UserClass *Object,
                                           PressedFuncType PressedFunc,
                                           ReleasedFuncType ReleasedFunc,
                                           TArray<uint32> &BindHandles) {
  check(InputConfig);

  for (const FRdInputAction &Action : InputConfig->AbilityInputActions) {
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
