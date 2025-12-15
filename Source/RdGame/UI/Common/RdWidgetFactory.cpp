// Copyright Epic Games, Inc. All Rights Reserved.

#include "RdWidgetFactory.h"
#include "Templates/SubclassOf.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RdWidgetFactory)

class UUserWidget;

TSubclassOf<UUserWidget> URdWidgetFactory::FindWidgetClassForData_Implementation(const UObject* Data) const
{
	return TSubclassOf<UUserWidget>();
}
