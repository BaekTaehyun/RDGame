#include "RdHUDLayout.h"
#include "Widgets/CommonActivatableWidgetContainer.h"

URdHUDLayout::URdHUDLayout(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {}

void URdHUDLayout::NativeOnInitialized() {
  Super::NativeOnInitialized();

  // Setup input handling for escape menu, etc.
  // This would typically be done via InputAction bindings in the widget
  // blueprint
}

void URdHUDLayout::HandleEscapeAction() {
  // Toggle pause menu or handle escape key logic
  // Can push a menu widget to MenuLayer
  if (MenuLayer) {
    // Example: Push pause menu
    // MenuLayer->AddWidget(PauseMenuClass);
  }
}

void URdHUDLayout::HandleInputMethodChanged() {
  // Handle input device change (show/hide controller prompts, etc.)
}
