#include "UIHelperFunctionLibrary.h"
#include "AssetToolsModule.h"
#include "WidgetBlueprintFactory.h"
#include "Blueprint/UserWidget.h"
#include "WidgetBlueprint.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "EditorAssetLibrary.h"

// UMG Includes
#include "Components/Button.h"
#include "Components/CanvasPanel.h"
#include "Components/CanvasPanelSlot.h"
#include "Blueprint/WidgetTree.h"

// JSON Includes
#include "Serialization/JsonSerializer.h"
#include "Policies/CondensedJsonPrintPolicy.h"

UWidgetBlueprint* UUIHelperFunctionLibrary::CreateWidgetBlueprint(FString AssetPath, FString AssetName, UClass* ParentClass)
{
	IAssetTools& AssetTools = FModuleManager::GetModuleChecked<FAssetToolsModule>("AssetTools").Get();

	FString PackageName = AssetPath + "/" + AssetName;
    
    // Check if asset already exists
    if (UEditorAssetLibrary::DoesAssetExist(PackageName))
    {
        return Cast<UWidgetBlueprint>(UEditorAssetLibrary::LoadAsset(PackageName));
    }

	UWidgetBlueprintFactory* Factory = NewObject<UWidgetBlueprintFactory>();
	if (ParentClass)
	{
		Factory->ParentClass = ParentClass;
	}
    else
    {
        Factory->ParentClass = UUserWidget::StaticClass();
    }

	UObject* NewAsset = AssetTools.CreateAsset(AssetName, AssetPath, UWidgetBlueprint::StaticClass(), Factory);

	return Cast<UWidgetBlueprint>(NewAsset);
}

UWidget* UUIHelperFunctionLibrary::AddWidgetToParent(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UWidget> WidgetClass, FString ParentName, FString WidgetName, FVector2D Position, FVector2D Size)
{
    if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree || !*WidgetClass)
    {
        return nullptr;
    }

    // Find the parent widget
    UWidget* ParentWidget = WidgetBlueprint->WidgetTree->FindWidget(FName(*ParentName));
    
    // Fallback: Check Root Widget
    if (!ParentWidget)
    {
        UWidget* RootWidget = WidgetBlueprint->WidgetTree->RootWidget;
        if (RootWidget && RootWidget->GetFName() == FName(*ParentName))
        {
            ParentWidget = RootWidget;
        }
        else if (!RootWidget)
        {
            // Case: Empty Tree.
            // If parent is requested as "CanvasPanel_0" or "Root", maybe create valid root?
            // For general safety, if no root exists and we are adding, we usually assume the first added widget becomes root
            // OR we create a default canvas.
            // Let's create a default root canvas if tree is empty.
             UCanvasPanel* NewCanvas = WidgetBlueprint->WidgetTree->ConstructWidget<UCanvasPanel>(UCanvasPanel::StaticClass(), FName("CanvasPanel_0"));
             WidgetBlueprint->WidgetTree->RootWidget = NewCanvas;
             ParentWidget = NewCanvas;
        }
        else
        {
             // Fallback search by string name just in case
             WidgetBlueprint->WidgetTree->ForEachWidget([&](UWidget* Widget) {
                if (Widget->GetFName().ToString() == ParentName)
                {
                    ParentWidget = Widget;
                }
            });
        }
    }

    UPanelWidget* ParentPanel = Cast<UPanelWidget>(ParentWidget);
    if (!ParentPanel)
    {
        // Maybe the ParentWidget isn't a panel (e.g. it's a Button or Image)?
        // We cannot add children to non-panels.
        return nullptr; 
    }

    // Create the Widget
    UWidget* NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UWidget>(WidgetClass, FName(*WidgetName));
    if (!NewWidget)
    {
        // Name conflict or failure
        NewWidget = WidgetBlueprint->WidgetTree->ConstructWidget<UWidget>(WidgetClass);
    }
    
    if (!NewWidget)
    {
        return nullptr;
    }

    // Add to Parent Panel
    UPanelSlot* NewSlot = ParentPanel->AddChild(NewWidget);
    
    // Special handling if parent is CanvasPanel to set Pos/Size
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(NewSlot))
    {
        CanvasSlot->SetPosition(Position);
        CanvasSlot->SetSize(Size);
    }
    
    // Mark dirty
    WidgetBlueprint->Modify();
    
    return NewWidget;
}

// Helper for recursion
void TraverseWidgetTree(UWidget* CurrentWidget, TSharedPtr<FJsonObject> CurrentJson)
{
    if (!CurrentWidget || !CurrentJson.IsValid()) return;

    // Properties
    CurrentJson->SetStringField("Name", CurrentWidget->GetName());
    CurrentJson->SetStringField("Class", CurrentWidget->GetClass()->GetName());
    
    // Geometry layout info (if slot)
    if (UCanvasPanelSlot* CanvasSlot = Cast<UCanvasPanelSlot>(CurrentWidget->Slot))
    {
        TSharedPtr<FJsonObject> SlotJson = MakeShareable(new FJsonObject);
        SlotJson->SetNumberField("X", CanvasSlot->GetPosition().X);
        SlotJson->SetNumberField("Y", CanvasSlot->GetPosition().Y);
        SlotJson->SetNumberField("SizeX", CanvasSlot->GetSize().X);
        SlotJson->SetNumberField("SizeY", CanvasSlot->GetSize().Y);
        CurrentJson->SetObjectField("Slot", SlotJson);
    }
    
    // Children
    TArray<TSharedPtr<FJsonValue>> ChildrenJson;
    
    // If it's a panel, iterate children
    if (UPanelWidget* Panel = Cast<UPanelWidget>(CurrentWidget))
    {
        for (int32 i = 0; i < Panel->GetChildrenCount(); ++i)
        {
            UWidget* Child = Panel->GetChildAt(i);
            if (Child)
            {
                TSharedPtr<FJsonObject> ChildObj = MakeShareable(new FJsonObject);
                TraverseWidgetTree(Child, ChildObj);
                ChildrenJson.Add(MakeShareable(new FJsonValueObject(ChildObj)));
            }
        }
    }
    // If it's a UserWidget (Nested), we generally treat it as a leaf unless we want to inspect inside
    
    if (ChildrenJson.Num() > 0)
    {
        CurrentJson->SetArrayField("Children", ChildrenJson);
    }
}

FString UUIHelperFunctionLibrary::GetWidgetTreeHierarchy(UWidgetBlueprint* WidgetBlueprint)
{
    if (!WidgetBlueprint || !WidgetBlueprint->WidgetTree)
    {
        return TEXT("{}");
    }

    TSharedPtr<FJsonObject> RootJson = MakeShareable(new FJsonObject);
    
    // Start with Root Widget
    UWidget* RootWidget = WidgetBlueprint->WidgetTree->RootWidget;
    if (RootWidget)
    {
        TraverseWidgetTree(RootWidget, RootJson);
    }
    else
    {
         RootJson->SetStringField("Error", "No Root Widget");
    }

    FString OutputString;
    TSharedRef<TJsonWriter<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>> Writer = TJsonWriterFactory<TCHAR, TCondensedJsonPrintPolicy<TCHAR>>::Create(&OutputString);
    FJsonSerializer::Serialize(RootJson.ToSharedRef(), Writer);
    
    return OutputString;
}
