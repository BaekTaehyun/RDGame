#include "MCPHelperWidget.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Components/Button.h"
#include "Blueprint/WidgetTree.h"
#include "WidgetBlueprint.h"
#include "Editor/UnrealEd/Public/FileHelpers.h"
#include "Editor/UnrealEd/Public/Editor.h"

UObject* UMCPHelperWidget::AddButtonToCanvas(UWidgetBlueprint* TargetBP, FName ParentCanvasName, FName ButtonName, FVector2D Position, FVector2D Size)
{
	if (!TargetBP || !TargetBP->WidgetTree)
	{
		return nullptr;
	}

	// 0. Ensure Root Widget exists
	if (!TargetBP->WidgetTree->RootWidget)
	{
		UCanvasPanel* RootCanvas = TargetBP->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName("RootCanvas"));
		TargetBP->WidgetTree->RootWidget = RootCanvas;
	}

	// 1. Find Parent Canvas
	UWidget* ParentWidget = TargetBP->WidgetTree->FindWidget(ParentCanvasName);
	if (!ParentWidget)
	{
		// Fallback 1: Use RootWidget if name matches "CanvasPanel_0" generic request or if ParentName is None/Root
		if (ParentCanvasName.IsNone() || ParentCanvasName == "CanvasPanel_0" || ParentCanvasName == "RootCanvas")
		{
			ParentWidget = TargetBP->WidgetTree->RootWidget;
		}
	}

	UCanvasPanel* CanvasPanel = Cast<UCanvasPanel>(ParentWidget);
	if (!CanvasPanel)
	{
		// Attempt to search children? No, simplistic for now.
		return nullptr;
	}

	// 2. Construct Button
	UButton* NewButton = TargetBP->WidgetTree->ConstructWidget<UButton>(UButton::StaticClass(), ButtonName);
	if (!NewButton)
	{
		return nullptr;
	}

	// 3. Add to Canvas
	UPanelSlot* NewSlot = CanvasPanel->AddChild(NewButton);
	UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NewSlot);

	// 4. Set Position/Size
	if (CanvasSlot)
	{
		CanvasSlot->SetPosition(Position);
		CanvasSlot->SetSize(Size);
	}

	// Mark as modified
	TargetBP->Modify();

	return NewButton;
}

bool UMCPHelperWidget::SaveAsset(UObject* Asset)
{
	if (!Asset) return false;

	UPackage* Package = Asset->GetOutermost();
	if (!Package) return false;

	FEditorFileUtils::PromptForCheckoutAndSave({ Package }, false, false);
	return true;
}
