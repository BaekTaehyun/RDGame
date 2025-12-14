#pragma once

#include "CommonActivatableWidget.h"
#include "CoreMinimal.h"
#include "RdActivatableWidget.generated.h"

struct FUIInputConfig;

UENUM(BlueprintType)
enum class ERdWidgetInputMode : uint8 { Default, GameAndMenu, Game, Menu };

/**
 * URdActivatableWidget
 *
 * Base class for activatable widgets in the RDGame project.
 * Handles input mode configuration similar to Lyra's implementation.
 */
UCLASS(Abstract, Blueprintable)
class RDGAME_API URdActivatableWidget : public UCommonActivatableWidget {
  GENERATED_BODY()

public:
  URdActivatableWidget(const FObjectInitializer &ObjectInitializer);

  //~UCommonActivatableWidget interface
  virtual TOptional<FUIInputConfig> GetDesiredInputConfig() const override;
  //~End of UCommonActivatableWidget interface

protected:
  /** The desired input mode to use while this widget is active. */
  UPROPERTY(EditDefaultsOnly, Category = Input)
  ERdWidgetInputMode InputConfig = ERdWidgetInputMode::Default;

  /** The desired mouse behavior when the widget is active. */
  UPROPERTY(EditDefaultsOnly, Category = Input)
  EMouseCaptureMode GameMouseCaptureMode =
      EMouseCaptureMode::CapturePermanently;
};
