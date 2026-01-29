#include "CoreMinimal.h"
#include "EditorUtilityWidget.h"
#include "MCPHelperWidget.generated.h"

class UWidgetBlueprint;

/**
 * Base class for MCP Helper Editor Utility Widget.
 * Exposes UMG manipulation functions to Blueprint/Python.
 */
UCLASS()
class UIHELPER_API UMCPHelperWidget : public UEditorUtilityWidget
{
	GENERATED_BODY()

public:
	/**
	 * Adds a Button to a CanvasPanel in the target WidgetBlueprint.
	 * @param TargetBP The WidgetBlueprint to modify.
	 * @param ParentCanvasName Name of the parent CanvasPanel widget (must exist).
	 * @param ButtonName Name for the new Button widget.
	 * @param Position X, Y position on the canvas.
	 * @param Size X, Y size of the button.
	 * @return The created Button widget (as UWidget object) or nullptr.
	 */
	UFUNCTION(BlueprintCallable, Category = "MCP|UMG")
	UObject* AddButtonToCanvas(UWidgetBlueprint* TargetBP, FName ParentCanvasName, FName ButtonName, FVector2D Position, FVector2D Size);

	/**
	 * Saves a modified asset.
	 */
	UFUNCTION(BlueprintCallable, Category = "MCP|Utils")
	bool SaveAsset(UObject* Asset);
};
