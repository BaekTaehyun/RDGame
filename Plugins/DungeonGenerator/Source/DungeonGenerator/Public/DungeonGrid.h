#pragma once

#include "CoreDungeonGrid.h"
#include "CoreMinimal.h"
#include "CoreBSPGenerator.h" // CoreMultiFloorConfig 정의를 위해 추가
#include "DungeonGrid.generated.h"


// Wrapper Enums
UENUM(BlueprintType)
enum class ETileType : uint8 {
  None UMETA(DisplayName = "None"),
  Floor UMETA(DisplayName = "Floor"),
  Wall UMETA(DisplayName = "Wall"),
  Corridor UMETA(DisplayName = "Corridor"),
  Door UMETA(DisplayName = "Door"),
  Stair UMETA(DisplayName = "Stair"),
  Debug UMETA(DisplayName = "Debug")
};

// Conversion Helpers
namespace DungeonConverter {
inline DungeonCore::ETileType ToCore(ETileType Type) {
  return static_cast<DungeonCore::ETileType>(Type);
}
inline ETileType ToUnreal(DungeonCore::ETileType Type) {
  return static_cast<ETileType>(Type);
}
} // namespace DungeonConverter

USTRUCT(BlueprintType)
struct FDungeonTile {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 X = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 Y = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  ETileType Type = ETileType::None;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 RoomID = -1;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  float MonsterSuitability = 0.0f;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 StairTargetFloor = -1;

  // Conversion Constructor
  FDungeonTile() = default;
  FDungeonTile(const DungeonCore::FCoreTile &CoreTile)
      : X(CoreTile.X), Y(CoreTile.Y),
        Type(DungeonConverter::ToUnreal(CoreTile.Type)),
        RoomID(CoreTile.RoomID),
        MonsterSuitability(CoreTile.MonsterSuitability),
        StairTargetFloor(CoreTile.StairTargetFloor) {}
};

USTRUCT(BlueprintType)
struct FDungeonGrid {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 Width = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 Height = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TArray<FDungeonTile> Tiles;

  void Init(int32 InWidth, int32 InHeight,
            ETileType InitialType = ETileType::Wall) {
    Width = InWidth;
    Height = InHeight;
    Tiles.SetNum(Width * Height);

    for (int32 Y = 0; Y < Height; Y++) {
      for (int32 X = 0; X < Width; X++) {
        int32 Index = Y * Width + X;
        Tiles[Index].X = X;
        Tiles[Index].Y = Y;
        Tiles[Index].Type = InitialType;
        Tiles[Index].RoomID = -1;
        Tiles[Index].StairTargetFloor = -1;
      }
    }
  }

  // Convert to Core Grid
  DungeonCore::CoreDungeonGrid ToCore() const {
    DungeonCore::CoreDungeonGrid CoreGrid(
        Width, Height, DungeonConverter::ToCore(ETileType::Wall));
    for (int32 i = 0; i < Tiles.Num(); ++i) {
      CoreGrid.Tiles[i].X = Tiles[i].X;
      CoreGrid.Tiles[i].Y = Tiles[i].Y;
      CoreGrid.Tiles[i].Type = DungeonConverter::ToCore(Tiles[i].Type);
      CoreGrid.Tiles[i].RoomID = Tiles[i].RoomID;
      CoreGrid.Tiles[i].MonsterSuitability = Tiles[i].MonsterSuitability;
      CoreGrid.Tiles[i].StairTargetFloor = Tiles[i].StairTargetFloor;
    }
    return CoreGrid;
  }

  // Update from Core Grid
  void FromCore(const DungeonCore::CoreDungeonGrid &CoreGrid) {
    Width = CoreGrid.Width;
    Height = CoreGrid.Height;
    Tiles.SetNum(Width * Height);

    for (int32 i = 0; i < CoreGrid.Tiles.size(); ++i) {
      Tiles[i] = FDungeonTile(CoreGrid.Tiles[i]);
    }
  }

  FDungeonTile &GetTile(int32 X, int32 Y) {
    // Simple bounds check could be added here
    return Tiles[Y * Width + X];
  }

  const FDungeonTile &GetTile(int32 X, int32 Y) const {
    return Tiles[Y * Width + X];
  }

  bool IsValid(int32 X, int32 Y) const {
    return X >= 0 && X < Width && Y >= 0 && Y < Height;
  }
};

// Multi-Floor Dungeon Structures (Unreal Wrappers)
USTRUCT(BlueprintType)
struct FStairPosition {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 FloorIndex = 0; // Floor this stair is on

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 X = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 Y = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 TargetFloor = 0; // Floor this stair leads to
};

USTRUCT(BlueprintType)
struct FMultiFloorDungeonConfig {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|MultiFloor")
  int32 NumFloors = 1;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|MultiFloor")
  int32 Width = 50;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|MultiFloor")
  int32 Height = 50;

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|MultiFloor")
  bool bEnforceVerticalAlignment = true; // Upper floors must have floor below

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|MultiFloor")
  int32 StairsPerFloor = 2; // Number of stair connections per floor

  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dungeon|MultiFloor")
  float MinStairDistance = 15.0f; // Minimum distance between stairs

  DungeonCore::CoreMultiFloorConfig ToCore() const {
    DungeonCore::CoreMultiFloorConfig CoreConfig;
    CoreConfig.NumFloors = NumFloors;
    CoreConfig.Width = Width;
    CoreConfig.Height = Height;
    CoreConfig.bEnforceVerticalAlignment = bEnforceVerticalAlignment;
    CoreConfig.StairsPerFloor = StairsPerFloor;
    CoreConfig.MinStairDistance = MinStairDistance;
    return CoreConfig;
  }
};

USTRUCT(BlueprintType)
struct FMultiFloorDungeon {
  GENERATED_BODY()

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TArray<FDungeonGrid> Floors; // Floors[0] = Floor 1, Floors[N-1] = Top floor

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  int32 NumFloors = 0;

  UPROPERTY(EditAnywhere, BlueprintReadWrite)
  TArray<FStairPosition> Stairs;

  void FromCore(const DungeonCore::CoreMultiFloorDungeon &CoreDungeon) {
    NumFloors = CoreDungeon.NumFloors;
    Floors.SetNum(NumFloors);
    for (int32 i = 0; i < NumFloors; ++i) {
      Floors[i].FromCore(CoreDungeon.Floors[i]);
    }

    Stairs.Empty();
    for (const auto &CoreStair : CoreDungeon.Stairs) {
      FStairPosition Stair;
      Stair.FloorIndex = CoreStair.FloorIndex;
      Stair.X = CoreStair.X;
      Stair.Y = CoreStair.Y;
      Stair.TargetFloor = CoreStair.TargetFloor;
      Stairs.Add(Stair);
    }
  }

  FDungeonGrid &GetFloor(int32 FloorIndex) {
    check(FloorIndex >= 0 && FloorIndex < Floors.Num());
    return Floors[FloorIndex];
  }

  const FDungeonGrid &GetFloor(int32 FloorIndex) const {
    check(FloorIndex >= 0 && FloorIndex < Floors.Num());
    return Floors[FloorIndex];
  }

  ETileType GetTileAt(int32 FloorIndex, int32 X, int32 Y) const {
    if (FloorIndex >= 0 && FloorIndex < Floors.Num()) {
      if (Floors[FloorIndex].IsValid(X, Y)) {
        return Floors[FloorIndex].GetTile(X, Y).Type;
      }
    }
    return ETileType::Wall;
  }

  bool IsFloorTile(int32 FloorIndex, int32 X, int32 Y) const {
    ETileType Type = GetTileAt(FloorIndex, X, Y);
    return Type == ETileType::Floor || Type == ETileType::Corridor ||
           Type == ETileType::Stair;
  }
};
