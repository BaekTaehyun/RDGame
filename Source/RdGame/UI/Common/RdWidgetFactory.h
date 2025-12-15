// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "UObject/Object.h"

#include "RdWidgetFactory.generated.h"

template <class TClass> class TSubclassOf;

class UUserWidget;
struct FFrame;

/**
 * URdWidgetFactory
 *
 * Abstract factory for creating widgets based on data type.
 */
UCLASS(Abstract, Blueprintable, BlueprintType, EditInlineNew)
class RDGAME_API URdWidgetFactory : public UObject
{
	GENERATED_BODY()

public:
	URdWidgetFactory() { }

	UFUNCTION(BlueprintNativeEvent, Category = "Widget Factory")
	TSubclassOf<UUserWidget> FindWidgetClassForData(const UObject* Data) const;
};
