// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "UObject/ObjectMacros.h"
#include "UObject/Interface.h"

#include "IRdIndicatorWidget.generated.h"

class AActor;
class URdIndicatorDescriptor;

/**
 * Interface for widgets that display indicators.
 */
UINTERFACE(BlueprintType)
class URdIndicatorWidgetInterface : public UInterface
{
	GENERATED_BODY()
};

class IRdIndicatorWidgetInterface
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, Category = "Indicator")
	void BindIndicator(URdIndicatorDescriptor* Indicator);

	UFUNCTION(BlueprintNativeEvent, Category = "Indicator")
	void UnbindIndicator(const URdIndicatorDescriptor* Indicator);
};
