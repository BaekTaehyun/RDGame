#include "Rendering/DungeonNavigationBuilder.h"
#include "Engine/World.h"
#include "NavigationSystem.h"


UDungeonNavigationBuilder::UDungeonNavigationBuilder() {
  TileSize = 100.0f;
  WallHeight = 300.0f;
  bAutoGenerateNavMesh = true;
}

void UDungeonNavigationBuilder::BuildNavMesh(const FDungeonGrid &Grid,
                                             UWorld *World) {
  if (!World || !bAutoGenerateNavMesh) {
    return;
  }

  UNavigationSystemV1 *NavSys =
      FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
  if (!NavSys) {
    UE_LOG(LogTemp, Warning,
           TEXT("DungeonNavigationBuilder: NavigationSystem not found"));
    return;
  }

  // Bounds 계산
  FBox Bounds = CalculateNavBounds(Grid);

  // NavMesh 리빌드
  TriggerNavMeshRebuild(World, Bounds);

  UE_LOG(
      LogTemp, Log,
      TEXT("DungeonNavigationBuilder: NavMesh rebuild triggered for bounds %s"),
      *Bounds.ToString());
}

FBox UDungeonNavigationBuilder::CalculateNavBounds(
    const FDungeonGrid &Grid, FVector Offset) const {
  FVector Min(0, 0, 0);
  FVector Max(Grid.Width * TileSize, Grid.Height * TileSize, WallHeight);

  // 오프셋 적용
  Min += Offset;
  Max += Offset;

  // 약간의 여유 추가
  Min -= FVector(TileSize, TileSize, 0);
  Max += FVector(TileSize, TileSize, TileSize);

  return FBox(Min, Max);
}

void UDungeonNavigationBuilder::TriggerNavMeshRebuild(UWorld *World,
                                                      const FBox &Bounds) {
  UNavigationSystemV1 *NavSys =
      FNavigationSystem::GetCurrent<UNavigationSystemV1>(World);
  if (!NavSys) {
    return;
  }

  // 해당 영역의 NavMesh 다시 빌드
  // NavSys->OnNavigationBoundsUpdated(nullptr);

  // 또는 전체 리빌드
  NavSys->Build();


  UE_LOG(LogTemp, Log,
         TEXT("DungeonNavigationBuilder: NavMesh rebuild complete"));
}
