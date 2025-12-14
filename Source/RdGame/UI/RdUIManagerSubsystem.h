#pragma once

#include "CoreMinimal.h"
#include "GameUIManagerSubsystem.h"
#include "RdUIManagerSubsystem.generated.h"

/**
 * URdUIManagerSubsystem
 *
 * Manages the global UI state and coordinates with the gameplay layer.
 */
UCLASS()
class RDGAME_API URdUIManagerSubsystem : public UGameUIManagerSubsystem {
  GENERATED_BODY()

public:
  URdUIManagerSubsystem();

  //~UGameInstanceSubsystem interface
  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;
  //~End of UGameInstanceSubsystem interface
};
