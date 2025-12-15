// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "AsyncMixin.h"
#include "Blueprint/UserWidgetPool.h"
#include "Widgets/SPanel.h"

class FActiveTimerHandle;
class FArrangedChildren;
class FChildren;
class FPaintArgs;
class FReferenceCollector;
class FSlateRect;
class FSlateWindowElementList;
class FWidgetStyle;
class URdIndicatorDescriptor;
class URdIndicatorManagerComponent;
struct FSlateBrush;

/**
 * SRdActorCanvas
 *
 * Slate widget for rendering indicators attached to world actors.
 */
class SRdActorCanvas : public SPanel, public FAsyncMixin, public FGCObject
{
public:
	/** ActorCanvas-specific slot class */
	class FSlot : public TSlotBase<FSlot>
	{
	public: 

		FSlot(URdIndicatorDescriptor* InIndicator)
			: TSlotBase<FSlot>()
			, Indicator(InIndicator)
			, ScreenPosition(FVector2D::ZeroVector)
			, Depth(0)
			, Priority(0.f)
			, bIsIndicatorVisible(true)
			, bInFrontOfCamera(true)
			, bHasValidScreenPosition(false)
			, bDirty(true)
			, bWasIndicatorClamped(false)
			, bWasIndicatorClampedStatusChanged(false)
		{
		}

		SLATE_SLOT_BEGIN_ARGS(FSlot, TSlotBase<FSlot>)
		SLATE_SLOT_END_ARGS()
		using TSlotBase<FSlot>::Construct;

		bool GetIsIndicatorVisible() const { return bIsIndicatorVisible; }
		void SetIsIndicatorVisible(bool bVisible);

		FVector2D GetScreenPosition() const { return ScreenPosition; }
		void SetScreenPosition(FVector2D InScreenPosition);

		double GetDepth() const { return Depth; }
		void SetDepth(double InDepth);

		int32 GetPriority() const { return Priority; }
		void SetPriority(int32 InPriority);

		bool GetInFrontOfCamera() const { return bInFrontOfCamera; }
		void SetInFrontOfCamera(bool bInFront);

		bool HasValidScreenPosition() const { return bHasValidScreenPosition; }
		void SetHasValidScreenPosition(bool bValidScreenPosition);

		bool bIsDirty() const { return bDirty; }
		void ClearDirtyFlag() { bDirty = false; }

		bool WasIndicatorClamped() const { return bWasIndicatorClamped; }
		void SetWasIndicatorClamped(bool bWasClamped) const;

		bool WasIndicatorClampedStatusChanged() const { return bWasIndicatorClampedStatusChanged; }
		void ClearIndicatorClampedStatusChangedFlag() { bWasIndicatorClampedStatusChanged = false; }

	private:
		void RefreshVisibility();

		URdIndicatorDescriptor* Indicator;
		FVector2D ScreenPosition;
		double Depth;
		int32 Priority;

		uint8 bIsIndicatorVisible : 1;
		uint8 bInFrontOfCamera : 1;
		uint8 bHasValidScreenPosition : 1;
		uint8 bDirty : 1;
		
		mutable uint8 bWasIndicatorClamped : 1;
		mutable uint8 bWasIndicatorClampedStatusChanged : 1;

		friend class SRdActorCanvas;
	};

	/** ActorCanvas-specific arrow slot class */
	class FArrowSlot : public TSlotBase<FArrowSlot>
	{
	};

	/** Begin the arguments for this slate widget */
	SLATE_BEGIN_ARGS(SRdActorCanvas) {
		_Visibility = EVisibility::HitTestInvisible;
	}
		/** Indicates that we have a slot that this widget supports */
		SLATE_SLOT_ARGUMENT(SRdActorCanvas::FSlot, Slots)
	
	/** This always goes at the end */
	SLATE_END_ARGS()

	SRdActorCanvas()
		: CanvasChildren(this)
		, ArrowChildren(this)
		, AllChildren(this)
	{
		AllChildren.AddChildren(CanvasChildren);
		AllChildren.AddChildren(ArrowChildren);
	}

	void Construct(const FArguments& InArgs, const FLocalPlayerContext& InCtx, const FSlateBrush* ActorCanvasArrowBrush);

	// SWidget Interface
	virtual void OnArrangeChildren( const FGeometry& AllottedGeometry, FArrangedChildren& ArrangedChildren ) const override;
	virtual FVector2D ComputeDesiredSize(float) const override { return FVector2D::ZeroVector; }
	virtual FChildren* GetChildren() override { return &AllChildren; }
	virtual int32 OnPaint(const FPaintArgs& Args, const FGeometry& AllottedGeometry, const FSlateRect& MyCullingRect, FSlateWindowElementList& OutDrawElements, int32 LayerId, const FWidgetStyle& InWidgetStyle, bool bParentEnabled) const;
	// End SWidget

	void SetDrawElementsInOrder(bool bInDrawElementsInOrder) { bDrawElementsInOrder = bInDrawElementsInOrder; }

	virtual FString GetReferencerName() const override;
	virtual void AddReferencedObjects( FReferenceCollector& Collector ) override;
	
private:
	void OnIndicatorAdded(URdIndicatorDescriptor* Indicator);
	void OnIndicatorRemoved(URdIndicatorDescriptor* Indicator);

	void AddIndicatorForEntry(URdIndicatorDescriptor* Indicator);
	void RemoveIndicatorForEntry(URdIndicatorDescriptor* Indicator);

	using FScopedWidgetSlotArguments = TPanelChildren<FSlot>::FScopedWidgetSlotArguments;
	FScopedWidgetSlotArguments AddActorSlot(URdIndicatorDescriptor* Indicator);
	int32 RemoveActorSlot(const TSharedRef<SWidget>& SlotWidget);

	void SetShowAnyIndicators(bool bIndicators);
	EActiveTimerReturnType UpdateCanvas(double InCurrentTime, float InDeltaTime);

	/** Helper function for calculating the offset */
	void GetOffsetAndSize(const URdIndicatorDescriptor* Indicator,
		FVector2D& OutSize, 
		FVector2D& OutOffset,
		FVector2D& OutPaddingMin,
		FVector2D& OutPaddingMax) const;

	void UpdateActiveTimer();

private:
	TArray<TObjectPtr<URdIndicatorDescriptor>> AllIndicators;
	TArray<URdIndicatorDescriptor*> InactiveIndicators;
	
	FLocalPlayerContext LocalPlayerContext;
	TWeakObjectPtr<URdIndicatorManagerComponent> IndicatorComponentPtr;

	/** All the slots in this canvas */
	TPanelChildren<FSlot> CanvasChildren;
	mutable TPanelChildren<FArrowSlot> ArrowChildren;
	FCombinedChildren AllChildren;

	FUserWidgetPool IndicatorPool;

	const FSlateBrush* ActorCanvasArrowBrush = nullptr;

	mutable int32 NextArrowIndex = 0;
	mutable int32 ArrowIndexLastUpdate = 0;

	/** Whether to draw elements in the order they were added to canvas */
	bool bDrawElementsInOrder = false;

	bool bShowAnyIndicators = false;

	mutable TOptional<FGeometry> OptionalPaintGeometry;

	TSharedPtr<FActiveTimerHandle> TickHandle;
};
