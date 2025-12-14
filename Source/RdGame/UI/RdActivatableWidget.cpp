#include "RdActivatableWidget.h"
#include "CommonInputTypeEnum.h"

URdActivatableWidget::URdActivatableWidget(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}

TOptional<FUIInputConfig> URdActivatableWidget::GetDesiredInputConfig() const {
  switch (InputConfig) {
  case ERdWidgetInputMode::GameAndMenu:
    return FUIInputConfig(ECommonInputMode::All, GameMouseCaptureMode);
  case ERdWidgetInputMode::Game:
    return FUIInputConfig(ECommonInputMode::Game, GameMouseCaptureMode);
  case ERdWidgetInputMode::Menu:
    return FUIInputConfig(ECommonInputMode::Menu, EMouseCaptureMode::NoCapture);
  case ERdWidgetInputMode::Default:
  default:
    return TOptional<FUIInputConfig>();
  }
}
