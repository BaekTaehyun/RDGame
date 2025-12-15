#pragma once

#include "CoreMinimal.h"
#include "GameUIPolicy.h"
#include "RdGameUIPolicy.generated.h"

class UCommonLocalPlayer;
class UPrimaryGameLayout;

/**
 * URdGameUIPolicy
 *
 * Defines the UI policy for the RDGame project, including the primary layout
 * widget class.
 */
UCLASS(Blueprintable, Config = Game)
class RDGAME_API URdGameUIPolicy : public UGameUIPolicy {
  GENERATED_BODY()

public:
  URdGameUIPolicy();

  //~UGameUIPolicy interface
protected:
  virtual void OnRootLayoutAddedToViewport(UCommonLocalPlayer *LocalPlayer,
                                           UPrimaryGameLayout *Layout) override;
  virtual void
  OnRootLayoutRemovedFromViewport(UCommonLocalPlayer *LocalPlayer,
                                  UPrimaryGameLayout *Layout) override;
  virtual void OnRootLayoutReleased(UCommonLocalPlayer *LocalPlayer,
                                    UPrimaryGameLayout *Layout) override;
  //~End of UGameUIPolicy interface
};
