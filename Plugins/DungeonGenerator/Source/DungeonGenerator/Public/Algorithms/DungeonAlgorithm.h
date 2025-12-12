#pragma once

#include "CoreMinimal.h"
#include "UObject/NoExportTypes.h"
#include "DungeonGrid.h"
#include "DungeonAlgorithm.generated.h"

/**
 * Base class for dungeon generation algorithms.
 */
UCLASS(Abstract, BlueprintType, Blueprintable)
class DUNGEONGENERATOR_API UDungeonAlgorithm : public UObject {
  GENERATED_BODY()

public:
  /**
   * Generates the dungeon into the provided grid.
   * @param Grid The grid to modify.
   * @param RandomStream The random stream to use for deterministic generation.
   */
  virtual void Generate(FDungeonGrid &Grid, FRandomStream &RandomStream);
};
