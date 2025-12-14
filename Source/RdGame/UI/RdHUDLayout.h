#pragma once

#include "CoreMinimal.h"
#include "RdActivatableWidget.h"
#include "RdHUDLayout.generated.h"

class UCommonActivatableWidgetContainerBase;

/**
 * URdHUDLayout
 *
 * The main HUD layout widget for RDGame.
 * This widget serves as the root container for the player's HUD elements.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class RDGAME_API URdHUDLayout : public URdActivatableWidget {
  GENERATED_BODY()

public:
  URdHUDLayout(const FObjectInitializer &ObjectInitializer);

protected:
  virtual void NativeOnInitialized() override;

  /** Called when the escape action is triggered */
  void HandleEscapeAction();

  /** Called when the input device changes (keyboard/controller) */
  void HandleInputMethodChanged();

protected:
  /** Menu layer for pause menus, settings, etc. */
  UPROPERTY(BlueprintReadOnly, meta = (BindWidget, OptionalWidget = true),
            Category = "UI")
  TObjectPtr<UCommonActivatableWidgetContainerBase> MenuLayer;
};
