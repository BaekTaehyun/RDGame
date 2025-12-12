#include "ObjectPlacer.h"
#include "Async/ParallelFor.h"
#include "Containers/Queue.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

TArray<int32> UObjectPlacer::CalculateDistanceMap(const FDungeonGrid &Grid,
                                                  int32 StartX, int32 StartY) {
  TArray<int32> Distances;
  Distances.Init(-1, Grid.Tiles.Num());

  if (!Grid.IsValid(StartX, StartY)) {
    return Distances;
  }

  if (Grid.GetTile(StartX, StartY).Type == ETileType::Wall) {
    return Distances;
  }

  TQueue<FIntPoint> Queue;
  Queue.Enqueue(FIntPoint(StartX, StartY));
  Distances[StartY * Grid.Width + StartX] = 0;

  static constexpr int32 Dirs[4][2] = {{0, 1}, {0, -1}, {1, 0}, {-1, 0}};

  while (!Queue.IsEmpty()) {
    FIntPoint Current;
    Queue.Dequeue(Current);
    const int32 CurrentIdx = Current.Y * Grid.Width + Current.X;
    const int32 CurrentDist = Distances[CurrentIdx];

    for (const auto &Dir : Dirs) {
      const int32 NextX = Current.X + Dir[0];
      const int32 NextY = Current.Y + Dir[1];

      if (Grid.IsValid(NextX, NextY)) {
        const int32 NextIdx = NextY * Grid.Width + NextX;
        if (Distances[NextIdx] == -1 &&
            Grid.GetTile(NextX, NextY).Type != ETileType::Wall) {
          Distances[NextIdx] = CurrentDist + 1;
          Queue.Enqueue(FIntPoint(NextX, NextY));
        }
      }
    }
  }

  return Distances;
}

TArray<FVector2D>
UObjectPlacer::GeneratePoissonPoints(const FDungeonGrid &Grid, float MinDist,
                                     int32 MaxPoints,
                                     FRandomStream &RandomStream) {

  if (MinDist <= 0.0f || MaxPoints <= 0) {
    return TArray<FVector2D>();
  }

  const float CellSize = MinDist / FMath::Sqrt(2.0f);
  const int32 GridW = FMath::CeilToInt(Grid.Width / CellSize);
  const int32 GridH = FMath::CeilToInt(Grid.Height / CellSize);

  TArray<TArray<int32>> BackgroundGrid;
  BackgroundGrid.SetNum(GridH);
  for (int32 i = 0; i < GridH; i++)
    BackgroundGrid[i].Init(-1, GridW);

  TArray<FVector2D> SamplePoints;
  SamplePoints.Reserve(FMath::Min(MaxPoints, Grid.Width * Grid.Height / 4));

  TArray<FVector2D> Candidates;
  Candidates.Reserve(Grid.Width * Grid.Height);

  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      if (Grid.GetTile(X, Y).Type == ETileType::Floor) {
        Candidates.Add(FVector2D(X + 0.5f, Y + 0.5f));
      }
    }
  }

  if (Candidates.Num() == 0) {
    return SamplePoints;
  }

  // Shuffle
  for (int32 i = Candidates.Num() - 1; i > 0; i--) {
    const int32 j = RandomStream.RandRange(0, i);
    Candidates.Swap(i, j);
  }

  for (const FVector2D &Point : Candidates) {
    if (SamplePoints.Num() >= MaxPoints)
      break;

    const int32 GX = FMath::FloorToInt(Point.X / CellSize);
    const int32 GY = FMath::FloorToInt(Point.Y / CellSize);

    if (GX < 0 || GX >= GridW || GY < 0 || GY >= GridH)
      continue;

    bool bTooClose = false;

    const int32 StartX = FMath::Max(0, GX - 2);
    const int32 EndX = FMath::Min(GridW - 1, GX + 2);
    const int32 StartY = FMath::Max(0, GY - 2);
    const int32 EndY = FMath::Min(GridH - 1, GY + 2);

    for (int32 NY = StartY; NY <= EndY && !bTooClose; NY++) {
      for (int32 NX = StartX; NX <= EndX; NX++) {
        const int32 ExistingIdx = BackgroundGrid[NY][NX];
        if (ExistingIdx != -1) {
          const float D2 =
              FVector2D::DistSquared(Point, SamplePoints[ExistingIdx]);
          if (D2 < MinDist * MinDist) {
            bTooClose = true;
            break;
          }
        }
      }
    }

    if (!bTooClose) {
      SamplePoints.Add(Point);
      BackgroundGrid[GY][GX] = SamplePoints.Num() - 1;
    }
  }

  return SamplePoints;
}

bool UObjectPlacer::CheckPropDiversity(
    const FVector2D &Position, EPropType Type,
    const TMap<EPropType, TArray<FPropData>> &PropsByType,
    float AvoidSameTypeRadius) {
  const float RadiusSq = AvoidSameTypeRadius * AvoidSameTypeRadius;

  // Only check props of the same type - O(M/5) instead of O(M)
  if (const TArray<FPropData> *TypedProps = PropsByType.Find(Type)) {
    for (const FPropData &Prop : *TypedProps) {
      const FVector2D PropPos(Prop.X + 0.5f, Prop.Y + 0.5f);
      const float DistSq = FVector2D::DistSquared(Position, PropPos);

      if (DistSq < RadiusSq) {
        return false; // Same type too close
      }
    }
  }

  return true; // Diversity maintained
}

TArray<FPropData> UObjectPlacer::GenerateProps(
    const FDungeonGrid &Grid, const TArray<FPropConfig> &PropConfigs,
    FRandomStream &RandomStream, bool bEnforceDiversity) {
  TArray<FPropData> Props;
  int32 PropIDCounter = 0;

  if (PropConfigs.Num() == 0) {
    UE_LOG(LogTemp, Warning,
           TEXT("GenerateProps: No prop configurations provided"));
    return Props;
  }

  // Calculate total probability
  float TotalProb = 0.0f;
  for (const FPropConfig &Cfg : PropConfigs) {
    TotalProb += Cfg.Probability;
  }

  // Find minimum distance from all configs
  float MinGlobalDist = PropConfigs[0].MinDistance;
  for (const FPropConfig &Cfg : PropConfigs) {
    MinGlobalDist = FMath::Min(MinGlobalDist, Cfg.MinDistance);
  }

  // Generate candidate points
  constexpr int32 MaxPropPoints = 1000;
  TArray<FVector2D> Points =
      GeneratePoissonPoints(Grid, MinGlobalDist, MaxPropPoints, RandomStream);

  if (Points.Num() == 0) {
    return Props;
  }

  Props.Reserve(Points.Num());

  // Build type-specific prop cache for O(1) lookup
  TMap<EPropType, TArray<FPropData>> PropsByType;

  for (const FVector2D &Pt : Points) {
    const int32 X = FMath::FloorToInt(Pt.X);
    const int32 Y = FMath::FloorToInt(Pt.Y);

    if (!Grid.IsValid(X, Y))
      continue;

    // Select prop type based on probability
    const float Roll = RandomStream.GetFraction() * TotalProb;
    float Cumulative = 0.0f;
    const FPropConfig *SelectedConfig = &PropConfigs[0];

    for (const FPropConfig &Cfg : PropConfigs) {
      Cumulative += Cfg.Probability;
      if (Roll <= Cumulative) {
        SelectedConfig = &Cfg;
        break;
      }
    }

    // Check if prop fits
    bool bFits = true;
    for (int32 dy = 0; dy < SelectedConfig->Size && bFits; dy++) {
      for (int32 dx = 0; dx < SelectedConfig->Size; dx++) {
        if (!Grid.IsValid(X + dx, Y + dy) ||
            Grid.GetTile(X + dx, Y + dy).Type != ETileType::Floor) {
          bFits = false;
          break;
        }
      }
    }

    if (!bFits)
      continue;

    // Check diversity (avoid same type clustering) - Optimized lookup
    if (bEnforceDiversity) {
      if (!CheckPropDiversity(FVector2D(X + 0.5f, Y + 0.5f),
                              SelectedConfig->Type, PropsByType,
                              SelectedConfig->AvoidSameTypeRadius)) {
        continue; // Skip this prop to maintain diversity
      }
    }

    // Create prop
    FPropData NewProp;
    NewProp.ID = PropIDCounter++;
    NewProp.Type = SelectedConfig->Type;
    NewProp.X = X;
    NewProp.Y = Y;
    Props.Add(NewProp);

    // Add to type-specific cache
    PropsByType.FindOrAdd(NewProp.Type).Add(NewProp);
  }

  UE_LOG(LogTemp, Log, TEXT("GenerateProps: Placed %d props from %d points"),
         Props.Num(), Points.Num());

  return Props;
}

void UObjectPlacer::CalculateMonsterZones(FDungeonGrid &Grid, float CheckRadius,
                                          int32 MinOpenSpace) {
  TRACE_CPUPROFILER_EVENT_SCOPE(UObjectPlacer::CalculateMonsterZones);

  // Build spatial grid for floor tiles once - O(W × H)
  FSpatialGrid SpatialGrid(FMath::Max(1, FMath::FloorToInt(CheckRadius)),
                           Grid.Width);

  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      if (Grid.GetTile(X, Y).Type == ETileType::Floor) {
        SpatialGrid.AddTile(X, Y);
      }
    }
  }

  const float RadiusSq = CheckRadius * CheckRadius;
  const int32 TotalTiles = Grid.Width * Grid.Height;

  // Parallel processing - Process each tile independently
  ParallelFor(TotalTiles, [&](int32 Index) {
    const int32 X = Index % Grid.Width;
    const int32 Y = Index / Grid.Width;
    FDungeonTile &Tile = Grid.GetTile(X, Y);

    // Only calculate for floor tiles
    if (Tile.Type != ETileType::Floor) {
      Tile.MonsterSuitability = 0.0f;
      return;
    }

    // Query nearby tiles using spatial grid - O(k) where k ≈ 9 cells
    TArray<FIntPoint> NearbyTiles;
    SpatialGrid.QueryRadius(X, Y, CheckRadius, NearbyTiles);

    // Count open tiles within radius
    int32 OpenCount = 0;
    const FVector2D CenterPos(X + 0.5f, Y + 0.5f);

    for (const FIntPoint &Pt : NearbyTiles) {
      const FVector2D CheckPos(Pt.X + 0.5f, Pt.Y + 0.5f);
      const float DistSq = FVector2D::DistSquared(CenterPos, CheckPos);

      if (DistSq <= RadiusSq) {
        OpenCount++;
      }
    }

    // Calculate suitability (0.0 to 1.0)
    if (NearbyTiles.Num() > 0 && OpenCount >= MinOpenSpace) {
      const float Ratio = static_cast<float>(OpenCount) / NearbyTiles.Num();
      const float BonusRatio =
          static_cast<float>(OpenCount - MinOpenSpace) / MinOpenSpace;
      Tile.MonsterSuitability =
          FMath::Clamp(Ratio + BonusRatio * 0.2f, 0.0f, 1.0f);
    } else {
      Tile.MonsterSuitability = 0.0f;
    }
  });

  UE_LOG(LogTemp, Log,
         TEXT("CalculateMonsterZones: Optimized calculation completed for "
              "%dx%d grid"),
         Grid.Width, Grid.Height);
}

bool UObjectPlacer::ValidateClusterSpace(const FDungeonGrid &Grid,
                                         const FVector2D &Center, float Radius,
                                         const TArray<FPropData> &ExistingProps,
                                         float MinDistanceFromProps) {

  const int32 MinX = FMath::Max(0, FMath::FloorToInt(Center.X - Radius));
  const int32 MaxX =
      FMath::Min(Grid.Width - 1, FMath::CeilToInt(Center.X + Radius));
  const int32 MinY = FMath::Max(0, FMath::FloorToInt(Center.Y - Radius));
  const int32 MaxY =
      FMath::Min(Grid.Height - 1, FMath::CeilToInt(Center.Y + Radius));

  int32 FloorCount = 0;
  int32 TotalCount = 0;
  const float RadiusSq = Radius * Radius;

  for (int32 Y = MinY; Y <= MaxY; Y++) {
    for (int32 X = MinX; X <= MaxX; X++) {
      const FVector2D TilePos(X + 0.5f, Y + 0.5f);
      const float DistSq = FVector2D::DistSquared(Center, TilePos);
      if (DistSq <= RadiusSq) {
        TotalCount++;
        if (Grid.GetTile(X, Y).Type == ETileType::Floor) {
          FloorCount++;
        }
      }
    }
  }

  const float FloorRatio =
      TotalCount > 0 ? static_cast<float>(FloorCount) / TotalCount : 0.0f;
  if (FloorRatio < 0.7f) {
    return false;
  }

  // Check distance from props
  const float MinDistSq = MinDistanceFromProps * MinDistanceFromProps;
  for (const FPropData &Prop : ExistingProps) {
    const FVector2D PropPos(Prop.X + 0.5f, Prop.Y + 0.5f);
    if (FVector2D::DistSquared(Center, PropPos) < MinDistSq) {
      return false;
    }
  }

  return true;
}

TArray<FVector2D> UObjectPlacer::GenerateClusterPositions(
    const FDungeonGrid &Grid, const FVector2D &Center, float Radius,
    int32 Count, FRandomStream &RandomStream) {

  TArray<FVector2D> Positions;
  Positions.Reserve(Count);

  const float MinMonsterDist = 2.0f;
  int32 Attempts = 0;
  const int32 MaxAttempts = Count * 50;

  while (Positions.Num() < Count && Attempts < MaxAttempts) {
    Attempts++;

    const float Angle = RandomStream.GetFraction() * 2.0f * PI;
    const float R = FMath::Sqrt(RandomStream.GetFraction()) * Radius;
    const FVector2D NewPos =
        Center + FVector2D(FMath::Cos(Angle), FMath::Sin(Angle)) * R;

    const int32 X = FMath::FloorToInt(NewPos.X);
    const int32 Y = FMath::FloorToInt(NewPos.Y);

    if (!Grid.IsValid(X, Y) || Grid.GetTile(X, Y).Type != ETileType::Floor) {
      continue;
    }

    bool bTooClose = false;
    for (const FVector2D &ExistingPos : Positions) {
      if (FVector2D::DistSquared(NewPos, ExistingPos) <
          MinMonsterDist * MinMonsterDist) {
        bTooClose = true;
        break;
      }
    }

    if (!bTooClose) {
      Positions.Add(NewPos);
    }
  }

  return Positions;
}

TArray<FMonsterCluster> UObjectPlacer::FindMonsterClusterLocations(
    const FDungeonGrid &Grid, const FMonsterClusterConfig &Config,
    const TArray<FPropData> &ExistingProps, FRandomStream &RandomStream,
    int32 MaxClusters) {

  TArray<FMonsterCluster> Clusters;

  if (Config.ClusterRadius <= 0.0f || Config.MonsterCount <= 0) {
    return Clusters;
  }

  const float ClusterSpacing = Config.ClusterRadius * 2.5f;
  TArray<FVector2D> CandidateCenters = GeneratePoissonPoints(
      Grid, ClusterSpacing, MaxClusters * 3, RandomStream);

  for (const FVector2D &Center : CandidateCenters) {
    if (Clusters.Num() >= MaxClusters)
      break;

    if (!ValidateClusterSpace(Grid, Center, Config.ClusterRadius, ExistingProps,
                              Config.MinDistanceFromProps)) {
      continue;
    }

    TArray<FVector2D> MonsterPositions = GenerateClusterPositions(
        Grid, Center, Config.ClusterRadius, Config.MonsterCount, RandomStream);

    if (MonsterPositions.Num() < Config.MonsterCount / 2) {
      continue;
    }

    FMonsterCluster NewCluster;
    NewCluster.CenterPosition = Center;
    NewCluster.Radius = Config.ClusterRadius;
    NewCluster.MonsterPositions = MonsterPositions;
    NewCluster.ActualMonsterCount = MonsterPositions.Num();

    Clusters.Add(NewCluster);
  }

  UE_LOG(LogTemp, Log,
         TEXT("FindMonsterClusterLocations: Found %d valid clusters"),
         Clusters.Num());

  return Clusters;
}
