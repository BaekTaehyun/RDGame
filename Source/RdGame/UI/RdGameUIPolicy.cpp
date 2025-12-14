#include "RdGameUIPolicy.h"
#include "CommonLocalPlayer.h"
#include "PrimaryGameLayout.h"

URdGameUIPolicy::URdGameUIPolicy() {}

void URdGameUIPolicy::OnRootLayoutAddedToViewport(
    UCommonLocalPlayer *LocalPlayer, UPrimaryGameLayout *Layout) {
  Super::OnRootLayoutAddedToViewport(LocalPlayer, Layout);
  // Implementation hook for when layout is added
}

void URdGameUIPolicy::OnRootLayoutRemovedFromViewport(
    UCommonLocalPlayer *LocalPlayer, UPrimaryGameLayout *Layout) {
  Super::OnRootLayoutRemovedFromViewport(LocalPlayer, Layout);
  // Implementation hook for when layout is removed
}

void URdGameUIPolicy::OnRootLayoutReleased(UCommonLocalPlayer *LocalPlayer,
                                           UPrimaryGameLayout *Layout) {
  Super::OnRootLayoutReleased(LocalPlayer, Layout);
  // Implementation hook for cleanup
}
