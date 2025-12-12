#pragma once

#include "CoreMinimal.h"
#include "DungeonGrid.h"
#include "ObjectPlacer.generated.h"

UENUM(BlueprintType)
enum class EPropType : uint8 {
  Large UMETA(DisplayName = "Large"),
  Medium_A UMETA(DisplayName = "Medium A"),
  Medium_B UMETA(DisplayName = "Medium B"),
  Small_A UMETA(DisplayName = "Small A"),
  Small_B UMETA(DisplayName = "Small B")
};

USTRUCT(BlueprintType)
struct FPropData {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 ID = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  EPropType Type = EPropType::Small_A;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 X = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 Y = 0;
};

USTRUCT(BlueprintType)
struct FPropConfig {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  EPropType Type = EPropType::Small_A;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 Size = 1;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float Probability = 0.2f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float MinDistance = 4.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float AvoidSameTypeRadius = 8.0f;

  FPropConfig() {}
  FPropConfig(EPropType InType, int32 InSize, float InProb, float InMinDist,
              float InAvoidRadius)
      : Type(InType), Size(InSize), Probability(InProb), MinDistance(InMinDist),
        AvoidSameTypeRadius(InAvoidRadius) {}
};

USTRUCT(BlueprintType)
struct FMonsterClusterConfig {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 MonsterCount = 5;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float ClusterRadius = 10.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float MinDistanceFromProps = 5.0f;

  FMonsterClusterConfig() {}
  FMonsterClusterConfig(int32 Count, float Radius)
      : MonsterCount(Count), ClusterRadius(Radius) {}
};

USTRUCT(BlueprintType)
struct FMonsterCluster {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  FVector2D CenterPosition;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float Radius = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TArray<FVector2D> MonsterPositions;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 ActualMonsterCount = 0;
};

// Helper structure for spatial hashing optimization
struct FSpatialGrid {
  TMap<int32, TArray<FIntPoint>> Cells;
  int32 CellSize;
  int32 GridWidth;

  FSpatialGrid(int32 InCellSize, int32 InGridWidth)
      : CellSize(InCellSize), GridWidth(InGridWidth) {}

  int32 GetCellKey(int32 X, int32 Y) const {
    return (X / CellSize) * 10000 + (Y / CellSize);
  }

  void AddTile(int32 X, int32 Y) {
    Cells.FindOrAdd(GetCellKey(X, Y)).Add(FIntPoint(X, Y));
  }

  void QueryRadius(int32 X, int32 Y, float Radius,
                   TArray<FIntPoint> &OutTiles) const {
    const int32 CellRadius = FMath::CeilToInt(Radius / CellSize);
    const int32 CenterCellX = X / CellSize;
    const int32 CenterCellY = Y / CellSize;

    OutTiles.Reset();
    for (int32 dy = -CellRadius; dy <= CellRadius; dy++) {
      for (int32 dx = -CellRadius; dx <= CellRadius; dx++) {
        const int32 CheckX = (CenterCellX + dx) * CellSize;
        const int32 CheckY = (CenterCellY + dy) * CellSize;
        const int32 Key = GetCellKey(CheckX, CheckY);

        if (const TArray<FIntPoint> *Cell = Cells.Find(Key)) {
          OutTiles.Append(*Cell);
        }
      }
    }
  }
};

UCLASS()
class DUNGEONGENERATOR_API UObjectPlacer : public UObject {
  GENERATED_BODY()

public:
  UFUNCTION(BlueprintCallable, Category = "Dungeon|ObjectPlacer")
  static TArray<int32> CalculateDistanceMap(const FDungeonGrid &Grid,
                                            int32 StartX, int32 StartY);

  UFUNCTION(BlueprintCallable, Category = "Dungeon|ObjectPlacer")
  static TArray<FVector2D> GeneratePoissonPoints(const FDungeonGrid &Grid,
                                                 float MinDist, int32 MaxPoints,
                                                 FRandomStream &RandomStream);

  UFUNCTION(BlueprintCallable, Category = "Dungeon|ObjectPlacer")
  static TArray<FPropData> GenerateProps(const FDungeonGrid &Grid,
                                         const TArray<FPropConfig> &PropConfigs,
                                         FRandomStream &RandomStream,
                                         bool bEnforceDiversity = true);

  UFUNCTION(BlueprintCallable, Category = "Dungeon|ObjectPlacer")
  static void CalculateMonsterZones(FDungeonGrid &Grid,
                                    float CheckRadius = 10.0f,
                                    int32 MinOpenSpace = 50);

  UFUNCTION(BlueprintCallable, Category = "Dungeon|ObjectPlacer")
  static TArray<FMonsterCluster> FindMonsterClusterLocations(
      const FDungeonGrid &Grid, const FMonsterClusterConfig &Config,
      const TArray<FPropData> &ExistingProps, FRandomStream &RandomStream,
      int32 MaxClusters = 5);

private:
  static bool
  CheckPropDiversity(const FVector2D &Position, EPropType Type,
                     const TMap<EPropType, TArray<FPropData>> &PropsByType,
                     float AvoidSameTypeRadius);

  static bool ValidateClusterSpace(const FDungeonGrid &Grid,
                                   const FVector2D &Center, float Radius,
                                   const TArray<FPropData> &ExistingProps,
                                   float MinDistanceFromProps);

  static TArray<FVector2D>
  GenerateClusterPositions(const FDungeonGrid &Grid, const FVector2D &Center,
                           float Radius, int32 Count,
                           FRandomStream &RandomStream);
};
