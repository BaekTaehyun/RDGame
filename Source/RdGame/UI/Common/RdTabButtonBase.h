// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "RdTabListWidgetBase.h"
#include "Foundation/RdButtonBase.h"

#include "RdTabButtonBase.generated.h"

class UCommonLazyImage;
class UObject;
struct FFrame;
struct FSlateBrush;

/**
 * URdTabButtonBase
 *
 * Tab button that implements the tab button interface.
 */
UCLASS(Abstract, Blueprintable, meta = (DisableNativeTick))
class RDGAME_API URdTabButtonBase : public URdButtonBase, public IRdTabButtonInterface
{
	GENERATED_BODY()

public:

	void SetIconFromLazyObject(TSoftObjectPtr<UObject> LazyObject);
	void SetIconBrush(const FSlateBrush& Brush);

protected:

	UFUNCTION()
	virtual void SetTabLabelInfo_Implementation(const FRdTabDescriptor& TabLabelInfo) override;

private:

	UPROPERTY(meta = (BindWidgetOptional))
	TObjectPtr<UCommonLazyImage> LazyImage_Icon;
};
