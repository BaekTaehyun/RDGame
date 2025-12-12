#include "Algorithms/CellularAutomataGenerator.h"
#include "Containers/Queue.h"

void UCellularAutomataGenerator::Generate(FDungeonGrid &Grid,
                                          FRandomStream &RandomStream) {
  // Validation
  if (Grid.Width < 10 || Grid.Height < 10) {
    UE_LOG(LogTemp, Error,
           TEXT("CellularAutomata: Grid too small (%dx%d), minimum 10x10"),
           Grid.Width, Grid.Height);
    return;
  }

  if (FillPercent <= 0.0f || FillPercent >= 1.0f) {
    UE_LOG(LogTemp, Warning,
           TEXT("CellularAutomata: FillPercent %.2f out of range [0,1], "
                "clamping"),
           FillPercent);
    FillPercent = FMath::Clamp(FillPercent, 0.1f, 0.9f);
  }

  // 1. Random Fill
  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      // Edges are always walls for stability
      if (X == 0 || X == Grid.Width - 1 || Y == 0 || Y == Grid.Height - 1) {
        Grid.GetTile(X, Y).Type = ETileType::Wall;
      } else {
        const bool bIsWall = RandomStream.GetFraction() < FillPercent;
        Grid.GetTile(X, Y).Type = bIsWall ? ETileType::Wall : ETileType::Floor;
      }
    }
  }

  // 2. Smooth map over multiple iterations
  const int32 ClampedIterations = FMath::Clamp(SmoothIterations, 1, 20);
  for (int32 i = 0; i < ClampedIterations; i++) {
    SmoothMap(Grid);
  }

  // 3. Connect isolated regions
  ConnectRegions(Grid, RandomStream);

  UE_LOG(LogTemp, Log,
         TEXT("CellularAutomata: Generated dungeon with %d smooth iterations"),
         ClampedIterations);
}

void UCellularAutomataGenerator::SmoothMap(FDungeonGrid &Grid) {
  // Pre-allocate array for new tile types
  TArray<ETileType> NewTypes;
  NewTypes.SetNum(Grid.Width * Grid.Height);

  // Apply cellular automata rules
  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      const int32 WallCount = GetSurroundingWallCount(Grid, X, Y);
      const int32 Idx = Y * Grid.Width + X;

      // Classic 4-5 rule: more than 4 neighbors = wall, less than 4 = floor
      if (WallCount > 4) {
        NewTypes[Idx] = ETileType::Wall;
      } else if (WallCount < 4) {
        NewTypes[Idx] = ETileType::Floor;
      } else {
        // Exactly 4: keep current state
        NewTypes[Idx] = Grid.GetTile(X, Y).Type;
      }
    }
  }

  // Apply new types to grid
  for (int32 i = 0; i < NewTypes.Num(); i++) {
    Grid.Tiles[i].Type = NewTypes[i];
  }
}

int32 UCellularAutomataGenerator::GetSurroundingWallCount(
    const FDungeonGrid &Grid, int32 GridX, int32 GridY) {
  int32 WallCount = 0;

  // Check 3x3 neighborhood (8 neighbors)
  for (int32 NeighborY = GridY - 1; NeighborY <= GridY + 1; NeighborY++) {
    for (int32 NeighborX = GridX - 1; NeighborX <= GridX + 1; NeighborX++) {
      // Skip center tile
      if (NeighborX == GridX && NeighborY == GridY)
        continue;

      // In-bounds check
      if (NeighborX >= 0 && NeighborX < Grid.Width && NeighborY >= 0 &&
          NeighborY < Grid.Height) {
        if (Grid.GetTile(NeighborX, NeighborY).Type == ETileType::Wall) {
          WallCount++;
        }
      } else {
        // Out of bounds counts as wall (encourages walls at edges)
        WallCount++;
      }
    }
  }

  return WallCount;
}

void UCellularAutomataGenerator::ConnectRegions(FDungeonGrid &Grid,
                                                FRandomStream &RandomStream) {
  // Find all disconnected floor regions
  TArray<TArray<FDungeonTile>> Regions = GetRegions(Grid, ETileType::Floor);

  if (Regions.Num() <= 1) {
    UE_LOG(LogTemp, Log,
           TEXT("CellularAutomata: Single region found, no connection needed"));
    return;
  }

  UE_LOG(LogTemp, Log, TEXT("CellularAutomata: Connecting %d regions"),
         Regions.Num());

  // Sort regions by size (largest first) for stability
  Regions.Sort([](const TArray<FDungeonTile> &A,
                  const TArray<FDungeonTile> &B) { return A.Num() > B.Num(); });

  // Track connected regions
  TArray<TArray<FDungeonTile>> ConnectedRegions;
  ConnectedRegions.Add(Regions[0]); // Start with largest region

  // Connect each isolated region to the nearest connected region
  for (int32 i = 1; i < Regions.Num(); i++) {
    const TArray<FDungeonTile> &CurrentRegion = Regions[i];

    // Find nearest connection point
    float BestDistSq = MAX_FLT;
    FDungeonTile BestTileA;
    FDungeonTile BestTileB;
    bool bFoundConnection = false;

    // Search for closest tiles between current region and any connected region
    for (const FDungeonTile &TileA : CurrentRegion) {
      for (const TArray<FDungeonTile> &ConnectedRegion : ConnectedRegions) {
        for (const FDungeonTile &TileB : ConnectedRegion) {
          const float DistSq = FVector2D::DistSquared(
              FVector2D(TileA.X, TileA.Y), FVector2D(TileB.X, TileB.Y));
          if (DistSq < BestDistSq) {
            BestDistSq = DistSq;
            BestTileA = TileA;
            BestTileB = TileB;
            bFoundConnection = true;
          }
        }
      }
    }

    // Create passage between regions
    if (bFoundConnection) {
      CreatePassage(Grid, BestTileA, BestTileB);
      ConnectedRegions.Add(CurrentRegion);
    } else {
      UE_LOG(LogTemp, Warning,
             TEXT("CellularAutomata: Could not connect region %d"), i);
    }
  }
}

TArray<TArray<FDungeonTile>>
UCellularAutomataGenerator::GetRegions(const FDungeonGrid &Grid,
                                       ETileType TileType) {
  TArray<TArray<FDungeonTile>> Regions;
  TSet<int32> Visited;

  // Flood-fill to identify distinct regions
  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      const int32 Idx = Y * Grid.Width + X;
      if (!Visited.Contains(Idx) && Grid.GetTile(X, Y).Type == TileType) {
        TArray<FDungeonTile> NewRegion =
            GetRegionTiles(Grid, X, Y, TileType, Visited);
        if (NewRegion.Num() > 0) {
          Regions.Add(NewRegion);
        }
      }
    }
  }

  return Regions;
}

TArray<FDungeonTile> UCellularAutomataGenerator::GetRegionTiles(
    const FDungeonGrid &Grid, int32 StartX, int32 StartY, ETileType TileType,
    TSet<int32> &Visited) {
  TArray<FDungeonTile> Tiles;
  TQueue<FIntPoint> Queue;

  // BFS starting from seed point
  Queue.Enqueue(FIntPoint(StartX, StartY));
  Visited.Add(StartY * Grid.Width + StartX);

  static constexpr int32 Dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

  while (!Queue.IsEmpty()) {
    FIntPoint Point;
    Queue.Dequeue(Point);
    Tiles.Add(Grid.GetTile(Point.X, Point.Y));

    // Check 4-connected neighbors
    for (const auto &Dir : Dirs) {
      const int32 NX = Point.X + Dir[0];
      const int32 NY = Point.Y + Dir[1];
      const int32 NIdx = NY * Grid.Width + NX;

      if (Grid.IsValid(NX, NY) && !Visited.Contains(NIdx) &&
          Grid.GetTile(NX, NY).Type == TileType) {
        Visited.Add(NIdx);
        Queue.Enqueue(FIntPoint(NX, NY));
      }
    }
  }

  return Tiles;
}

void UCellularAutomataGenerator::CreatePassage(FDungeonGrid &Grid,
                                               const FDungeonTile &TileA,
                                               const FDungeonTile &TileB) {
  // Bresenham's line algorithm for straight passages
  int32 X = TileA.X;
  int32 Y = TileA.Y;
  const int32 EndX = TileB.X;
  const int32 EndY = TileB.Y;

  const int32 DX = FMath::Abs(EndX - X);
  const int32 DY = FMath::Abs(EndY - Y);
  const int32 SX = (X < EndX) ? 1 : -1;
  const int32 SY = (Y < EndY) ? 1 : -1;
  int32 Err = DX - DY;

  // Carve passage with slight width for better connectivity
  while (true) {
    Dig(Grid, X, Y);         // Center
    Dig(Grid, X + 1, Y);     // Right
    Dig(Grid, X, Y + 1);     // Down
    Dig(Grid, X + 1, Y + 1); // Diagonal (for smoother passages)

    if (X == EndX && Y == EndY)
      break;

    const int32 E2 = 2 * Err;
    if (E2 > -DY) {
      Err -= DY;
      X += SX;
    }
    if (E2 < DX) {
      Err += DX;
      Y += SY;
    }
  }
}

void UCellularAutomataGenerator::Dig(FDungeonGrid &Grid, int32 X, int32 Y) {
  if (Grid.IsValid(X, Y)) {
    const int32 Idx = Y * Grid.Width + X;
    Grid.Tiles[Idx].Type = ETileType::Floor;
  }
}
