#pragma once

#include "CoreMinimal.h"
#include "Algorithms/DungeonAlgorithm.h"

#include "CellularAutomataGenerator.generated.h"

/**
 * Cellular Automata Generator for cave-like dungeons.
 */
UCLASS()
class DUNGEONGENERATOR_API UCellularAutomataGenerator
    : public UDungeonAlgorithm {
  GENERATED_BODY()

public:
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CA Settings")
  int32 SmoothIterations = 5;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "CA Settings",
            meta = (ClampMin = "0.0", ClampMax = "1.0"))
  float FillPercent = 0.45f;

  virtual void Generate(FDungeonGrid &Grid,
                        FRandomStream &RandomStream) override;

private:
  void SmoothMap(FDungeonGrid &Grid);
  int32 GetSurroundingWallCount(const FDungeonGrid &Grid, int32 GridX,
                                int32 GridY);

  // Region Connection Logic
  void ConnectRegions(FDungeonGrid &Grid, FRandomStream &RandomStream);
  TArray<TArray<FDungeonTile>> GetRegions(const FDungeonGrid &Grid,
                                          ETileType TileType);
  TArray<FDungeonTile> GetRegionTiles(const FDungeonGrid &Grid, int32 StartX,
                                      int32 StartY, ETileType TileType,
                                      TSet<int32> &Visited);
  void CreatePassage(FDungeonGrid &Grid, const FDungeonTile &TileA,
                     const FDungeonTile &TileB);
  void Dig(FDungeonGrid &Grid, int32 X, int32 Y);
};
