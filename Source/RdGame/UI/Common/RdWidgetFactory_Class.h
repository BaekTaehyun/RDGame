// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RdWidgetFactory.h"
#include "Templates/SubclassOf.h"
#include "UObject/SoftObjectPtr.h"

#include "RdWidgetFactory_Class.generated.h"

class UObject;
class UUserWidget;

/**
 * URdWidgetFactory_Class
 *
 * Widget factory that maps object classes to widget classes.
 */
UCLASS()
class RDGAME_API URdWidgetFactory_Class : public URdWidgetFactory
{
	GENERATED_BODY()

public:
	URdWidgetFactory_Class() { }

	virtual TSubclassOf<UUserWidget> FindWidgetClassForData_Implementation(const UObject* Data) const override;
	
protected:
	UPROPERTY(EditAnywhere, Category = ListEntries, meta = (AllowAbstract))
	TMap<TSoftClassPtr<UObject>, TSubclassOf<UUserWidget>> EntryWidgetForClass;
};
