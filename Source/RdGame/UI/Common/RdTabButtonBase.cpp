// Copyright Epic Games, Inc. All Rights Reserved.

#include "RdTabButtonBase.h"

#include "CommonLazyImage.h"
#include "RdTabListWidgetBase.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RdTabButtonBase)

class UObject;
struct FSlateBrush;

void URdTabButtonBase::SetIconFromLazyObject(TSoftObjectPtr<UObject> LazyObject)
{
	if (LazyImage_Icon)
	{
		LazyImage_Icon->SetBrushFromLazyDisplayAsset(LazyObject);
	}
}

void URdTabButtonBase::SetIconBrush(const FSlateBrush& Brush)
{
	if (LazyImage_Icon)
	{
		LazyImage_Icon->SetBrush(Brush);
	}
}

void URdTabButtonBase::SetTabLabelInfo_Implementation(const FRdTabDescriptor& TabLabelInfo)
{
	SetButtonText(TabLabelInfo.TabText);
	SetIconBrush(TabLabelInfo.IconBrush);
}
