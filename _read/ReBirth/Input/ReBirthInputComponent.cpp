// Copyright Epic Games, Inc. All Rights Reserved.

#include "Input/ReBirthInputComponent.h"

#include "EnhancedInputSubsystems.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(ReBirthInputComponent)

UReBirthInputComponent::UReBirthInputComponent(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}

void UReBirthInputComponent::AddInputMappings(
    const UReBirthInputConfig *InputConfig,
    UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const {
  check(InputConfig);
  check(InputSubsystem);

  // Here you can handle any custom logic to add something from your input
  // config if required
}

void UReBirthInputComponent::RemoveInputMappings(
    const UReBirthInputConfig *InputConfig,
    UEnhancedInputLocalPlayerSubsystem *InputSubsystem) const {
  check(InputConfig);
  check(InputSubsystem);

  // Here you can handle any custom logic to remove input mappings that you may
  // have added above
}

void UReBirthInputComponent::RemoveBinds(TArray<uint32> &BindHandles) {
  for (uint32 Handle : BindHandles) {
    RemoveBindingByHandle(Handle);
  }
  BindHandles.Reset();
}
