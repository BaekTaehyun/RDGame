#include "Input/RdInputComponent.h"
#include "EnhancedInputSubsystems.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RdInputComponent)

URdInputComponent::URdInputComponent(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}

void URdInputComponent::AddInputMappings(
    const URdInputConfig *InputConfig,
    UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const {
  check(InputConfig);
  check(InputSubsystem);
  // 커스텀 매핑 로직이 필요하면 여기에 추가 (예: IMC 동적 추가)
}

void URdInputComponent::RemoveInputMappings(
    const URdInputConfig *InputConfig,
    UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const {
  check(InputConfig);
  check(InputSubsystem);
  // 커스텀 제거 로직이 필요하면 여기에 추가
}

void URdInputComponent::RemoveBinds(TArray<uint32> &BindHandles) {
  for (uint32 Handle : BindHandles) {
    RemoveBindingByHandle(Handle);
  }
  BindHandles.Reset();
}
