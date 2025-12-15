// Copyright Epic Games, Inc. All Rights Reserved.

#include "RdIndicatorManagerComponent.h"

#include "RdIndicatorDescriptor.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RdIndicatorManagerComponent)

URdIndicatorManagerComponent::URdIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer)
	: Super(ObjectInitializer)
{
	bAutoRegister = true;
	bAutoActivate = true;
}

/*static*/ URdIndicatorManagerComponent* URdIndicatorManagerComponent::GetComponent(AController* Controller)
{
	if (Controller)
	{
		return Controller->FindComponentByClass<URdIndicatorManagerComponent>();
	}

	return nullptr;
}

void URdIndicatorManagerComponent::AddIndicator(URdIndicatorDescriptor* IndicatorDescriptor)
{
	IndicatorDescriptor->SetIndicatorManagerComponent(this);
	OnIndicatorAdded.Broadcast(IndicatorDescriptor);
	Indicators.Add(IndicatorDescriptor);
}

void URdIndicatorManagerComponent::RemoveIndicator(URdIndicatorDescriptor* IndicatorDescriptor)
{
	if (IndicatorDescriptor)
	{
		ensure(IndicatorDescriptor->GetIndicatorManagerComponent() == this);
	
		OnIndicatorRemoved.Broadcast(IndicatorDescriptor);
		Indicators.Remove(IndicatorDescriptor);
	}
}
