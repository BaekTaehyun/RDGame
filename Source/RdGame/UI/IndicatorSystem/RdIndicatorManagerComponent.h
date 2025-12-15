// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/ControllerComponent.h"

#include "RdIndicatorManagerComponent.generated.h"

class AController;
class URdIndicatorDescriptor;
class UObject;
struct FFrame;

/**
 * URdIndicatorManagerComponent
 *
 * Component for managing indicators on a controller.
 */
UCLASS(BlueprintType, Blueprintable)
class RDGAME_API URdIndicatorManagerComponent : public UControllerComponent
{
	GENERATED_BODY()

public:
	URdIndicatorManagerComponent(const FObjectInitializer& ObjectInitializer);

	static URdIndicatorManagerComponent* GetComponent(AController* Controller);

	UFUNCTION(BlueprintCallable, Category = Indicator)
	void AddIndicator(URdIndicatorDescriptor* IndicatorDescriptor);
	
	UFUNCTION(BlueprintCallable, Category = Indicator)
	void RemoveIndicator(URdIndicatorDescriptor* IndicatorDescriptor);

	DECLARE_EVENT_OneParam(URdIndicatorManagerComponent, FIndicatorEvent, URdIndicatorDescriptor* Descriptor)
	FIndicatorEvent OnIndicatorAdded;
	FIndicatorEvent OnIndicatorRemoved;

	const TArray<URdIndicatorDescriptor*>& GetIndicators() const { return Indicators; }

private:
	UPROPERTY()
	TArray<TObjectPtr<URdIndicatorDescriptor>> Indicators;
};
