// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Components/SceneComponent.h"
#include "Types/SlateEnums.h"

#include "RdIndicatorDescriptor.generated.h"

class SWidget;
class URdIndicatorDescriptor;
class URdIndicatorManagerComponent;
class UUserWidget;
struct FFrame;
struct FSceneViewProjectionData;

/** Projection helper for indicator descriptors */
struct RDGAME_API FRdIndicatorProjection
{
	bool Project(const URdIndicatorDescriptor& IndicatorDescriptor, const FSceneViewProjectionData& InProjectionData, const FVector2f& ScreenSize, FVector& ScreenPositionWithDepth);
};

UENUM(BlueprintType)
enum class ERdIndicatorProjectionMode : uint8
{
	ComponentPoint,
	ComponentBoundingBox,
	ComponentScreenBoundingBox,
	ActorBoundingBox,
	ActorScreenBoundingBox
};

/**
 * URdIndicatorDescriptor
 *
 * Describes and controls an active indicator attached to a scene component.
 */
UCLASS(BlueprintType)
class RDGAME_API URdIndicatorDescriptor : public UObject
{
	GENERATED_BODY()
	
public:
	URdIndicatorDescriptor() { }

public:
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	UObject* GetDataObject() const { return DataObject; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetDataObject(UObject* InDataObject) { DataObject = InDataObject; }
	
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	USceneComponent* GetSceneComponent() const { return Component; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetSceneComponent(USceneComponent* InComponent) { Component = InComponent; }

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	FName GetComponentSocketName() const { return ComponentSocketName; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetComponentSocketName(FName SocketName) { ComponentSocketName = SocketName; }

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	TSoftClassPtr<UUserWidget> GetIndicatorClass() const { return IndicatorWidgetClass; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetIndicatorClass(TSoftClassPtr<UUserWidget> InIndicatorWidgetClass)
	{
		IndicatorWidgetClass = InIndicatorWidgetClass;
	}

public:
	TWeakObjectPtr<UUserWidget> IndicatorWidget;
	TWeakPtr<SWidget> CanvasHost;

public:
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetAutoRemoveWhenIndicatorComponentIsNull(bool CanAutomaticallyRemove)
	{
		bAutoRemoveWhenIndicatorComponentIsNull = CanAutomaticallyRemove;
	}
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	bool GetAutoRemoveWhenIndicatorComponentIsNull() const { return bAutoRemoveWhenIndicatorComponentIsNull; }

	bool CanAutomaticallyRemove() const
	{
		return bAutoRemoveWhenIndicatorComponentIsNull && !IsValid(GetSceneComponent());
	}

public:
	// Layout Properties
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	bool GetIsVisible() const { return IsValid(GetSceneComponent()) && bVisible; }
	
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetDesiredVisibility(bool InVisible)
	{
		bVisible = InVisible;
	}

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	ERdIndicatorProjectionMode GetProjectionMode() const { return ProjectionMode; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetProjectionMode(ERdIndicatorProjectionMode InProjectionMode)
	{
		ProjectionMode = InProjectionMode;
	}

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	EHorizontalAlignment GetHAlign() const { return HAlignment; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetHAlign(EHorizontalAlignment InHAlignment)
	{
		HAlignment = InHAlignment;
	}

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	EVerticalAlignment GetVAlign() const { return VAlignment; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetVAlign(EVerticalAlignment InVAlignment)
	{
		VAlignment = InVAlignment;
	}

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	bool GetClampToScreen() const { return bClampToScreen; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetClampToScreen(bool bValue)
	{
		bClampToScreen = bValue;
	}

	// Show the arrow if clamping to the edge of the screen
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	bool GetShowClampToScreenArrow() const { return bShowClampToScreenArrow; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetShowClampToScreenArrow(bool bValue)
	{
		bShowClampToScreenArrow = bValue;
	}

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	FVector GetWorldPositionOffset() const { return WorldPositionOffset; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetWorldPositionOffset(FVector Offset)
	{
		WorldPositionOffset = Offset;
	}

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	FVector2D GetScreenSpaceOffset() const { return ScreenSpaceOffset; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetScreenSpaceOffset(FVector2D Offset)
	{
		ScreenSpaceOffset = Offset;
	}

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	FVector GetBoundingBoxAnchor() const { return BoundingBoxAnchor; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetBoundingBoxAnchor(FVector InBoundingBoxAnchor)
	{
		BoundingBoxAnchor = InBoundingBoxAnchor;
	}

	UFUNCTION(BlueprintCallable, Category = "Indicator")
	int32 GetPriority() const { return Priority; }
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void SetPriority(int32 InPriority)
	{
		Priority = InPriority;
	}

public:
	URdIndicatorManagerComponent* GetIndicatorManagerComponent() { return ManagerPtr.Get(); }
	void SetIndicatorManagerComponent(URdIndicatorManagerComponent* InManager);
	
	UFUNCTION(BlueprintCallable, Category = "Indicator")
	void UnregisterIndicator();

private:
	friend class SRdActorCanvas;

	UPROPERTY()
	bool bVisible = true;
	UPROPERTY()
	bool bClampToScreen = false;
	UPROPERTY()
	bool bShowClampToScreenArrow = false;
	UPROPERTY()
	bool bAutoRemoveWhenIndicatorComponentIsNull = false;

	UPROPERTY()
	ERdIndicatorProjectionMode ProjectionMode = ERdIndicatorProjectionMode::ComponentPoint;
	UPROPERTY()
	TEnumAsByte<EHorizontalAlignment> HAlignment = HAlign_Center;
	UPROPERTY()
	TEnumAsByte<EVerticalAlignment> VAlignment = VAlign_Center;

	UPROPERTY()
	int32 Priority = 0;

	UPROPERTY()
	FVector BoundingBoxAnchor = FVector(0.5, 0.5, 0.5);
	UPROPERTY()
	FVector2D ScreenSpaceOffset = FVector2D(0, 0);
	UPROPERTY()
	FVector WorldPositionOffset = FVector(0, 0, 0);

private:
	UPROPERTY()
	TObjectPtr<UObject> DataObject;
	
	UPROPERTY()
	TObjectPtr<USceneComponent> Component;

	UPROPERTY()
	FName ComponentSocketName = NAME_None;

	UPROPERTY()
	TSoftClassPtr<UUserWidget> IndicatorWidgetClass;

	UPROPERTY()
	TWeakObjectPtr<URdIndicatorManagerComponent> ManagerPtr;

	TWeakPtr<SWidget> Content;
};
