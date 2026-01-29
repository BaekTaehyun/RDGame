#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "WidgetBlueprint.h"
#include "UIHelperFunctionLibrary.generated.h"

/**
 * Helper library to expose UMG Editor functionality to Python/Blueprints
 */
UCLASS()
class UIHELPER_API UUIHelperFunctionLibrary : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Creates a new Widget Blueprint asset at the specified path.
	 * @param AssetPath The package path (e.g. "/Game/UI/MyWidget")
	 * @param AssetName The name of the asset (e.g. "WBP_MyWidget")
	 * @param ParentClass The parent class for the widget (defaults to UserWidget)
	 * @return The created Widget Blueprint asset, or nullptr on failure
	 */
	UFUNCTION(BlueprintCallable, Category = "UI Helper")
	static UWidgetBlueprint* CreateWidgetBlueprint(FString AssetPath, FString AssetName, UClass* ParentClass = nullptr);

    /**
     * Adds any widget class to a parent widget (Panel) in the Widget Blueprint.
     * Supports CanvasPanel, VerticalBox, Overlay, etc.
     * @param WidgetBlueprint The target blueprint
     * @param WidgetClass The class of the widget to create
     * @param ParentName The name of the parent widget
     * @param WidgetName The name for the new widget
     * @param Position Position (Effective only if parent is CanvasPanel)
     * @param Size Size (Effective only if parent is CanvasPanel)
     * @return The created widget object, or nullptr
     */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    static UWidget* AddWidgetToParent(UWidgetBlueprint* WidgetBlueprint, TSubclassOf<UWidget> WidgetClass, FString ParentName, FString WidgetName, FVector2D Position, FVector2D Size);
    /**
     * Inspects the widget tree of a Widget Blueprint and returns its hierarchy as a JSON string.
     * @param WidgetBlueprint The target blueprint
     * @return JSON string string representing the hierarchy
     */
    UFUNCTION(BlueprintCallable, Category = "UI Helper")
    static FString GetWidgetTreeHierarchy(UWidgetBlueprint* WidgetBlueprint);
};
