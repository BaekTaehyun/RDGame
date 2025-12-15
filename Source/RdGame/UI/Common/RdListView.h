// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonListView.h"

#include "RdListView.generated.h"

class UUserWidget;
class ULocalPlayer;
class URdWidgetFactory;

/**
 * URdListView
 *
 * Extended list view with factory-based widget creation.
 */
UCLASS(meta = (DisableNativeTick))
class RDGAME_API URdListView : public UCommonListView
{
	GENERATED_BODY()

public:
	URdListView(const FObjectInitializer& ObjectInitializer = FObjectInitializer::Get());

#if WITH_EDITOR
	virtual void ValidateCompiledDefaults(IWidgetCompilerLog& InCompileLog) const override;
#endif

protected:
	virtual UUserWidget& OnGenerateEntryWidgetInternal(UObject* Item, TSubclassOf<UUserWidget> DesiredEntryClass, const TSharedRef<STableViewBase>& OwnerTable) override;

protected:
	UPROPERTY(EditAnywhere, Instanced, Category="Entry Creation")
	TArray<TObjectPtr<URdWidgetFactory>> FactoryRules;
};
