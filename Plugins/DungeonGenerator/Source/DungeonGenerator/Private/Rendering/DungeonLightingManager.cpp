#include "Rendering/DungeonLightingManager.h"
#include "Components/PointLightComponent.h"
#include "Engine/World.h"
#include "Engine/PointLight.h"

UDungeonLightingManager::UDungeonLightingManager() {
  CorridorLightSpacing = 500.0f;
  BaseLightIntensity = 3000.0f;
  LightAttenuationRadius = 1000.0f;
  LightHeightRatio = 0.7f;
  TileSize = 100.0f;
  WallHeight = 300.0f;
}

void UDungeonLightingManager::PlaceDungeonLights(
    const FDungeonGrid &Grid, UWorld *World, TArray<APointLight *> &OutLights) {
  if (!World) {
    UE_LOG(LogTemp, Error, TEXT("DungeonLightingManager: World is null"));
    return;
  }

  OutLights.Empty();

  // 방 조명
  PlaceRoomLights(Grid, World, OutLights);

  // 복도 조명
  PlaceCorridorLights(Grid, World, OutLights);

  UE_LOG(LogTemp, Log, TEXT("DungeonLightingManager: Placed %d lights total"),
         OutLights.Num());
}

void UDungeonLightingManager::PlaceRoomLights(
    const FDungeonGrid &Grid, UWorld *World, TArray<APointLight *> &OutLights) {
  TArray<FIntPoint> RoomCenters = FindRoomCenters(Grid);

  for (const FIntPoint &Center : RoomCenters) {
    FVector Location(Center.X * TileSize, Center.Y * TileSize,
                     WallHeight * LightHeightRatio);

    // 방 크기에 따라 강도 조정 (간소화: 기본 강도 사용)
    float Intensity = BaseLightIntensity * 1.5f; // 방은 더 밝게

    APointLight *Light = SpawnLight(World, Location, Intensity, RoomLightClass);
    if (Light) {
      OutLights.Add(Light);
    }
  }

  UE_LOG(LogTemp, Log, TEXT("DungeonLightingManager: Placed %d room lights"),
         RoomCenters.Num());
}

void UDungeonLightingManager::PlaceCorridorLights(
    const FDungeonGrid &Grid, UWorld *World, TArray<APointLight *> &OutLights) {
  TArray<FIntPoint> CorridorTiles = FindCorridorTiles(Grid);

  // 간격을 두고 배치
  int32 LightCount = 0;
  for (int32 i = 0; i < CorridorTiles.Num();
       i += FMath::Max(1, FMath::FloorToInt(CorridorLightSpacing / TileSize))) {
    const FIntPoint &Tile = CorridorTiles[i];
    FVector Location(Tile.X * TileSize, Tile.Y * TileSize,
                     WallHeight * LightHeightRatio);

    APointLight *Light =
        SpawnLight(World, Location, BaseLightIntensity, CorridorLightClass);
    if (Light) {
      OutLights.Add(Light);
      LightCount++;
    }
  }

  UE_LOG(LogTemp, Log,
         TEXT("DungeonLightingManager: Placed %d corridor lights"), LightCount);
}

void UDungeonLightingManager::PlaceCaveLights(
    const FDungeonGrid &Grid, UWorld *World, TArray<APointLight *> &OutLights) {
  // 동굴 조명: 바닥 타일 전체에서 일정 간격으로 배치
  int32 StepSize =
      FMath::Max(5, FMath::FloorToInt(CorridorLightSpacing / TileSize));

  for (int32 Y = StepSize / 2; Y < Grid.Height; Y += StepSize) {
    for (int32 X = StepSize / 2; X < Grid.Width; X += StepSize) {
      if (Grid.GetTile(X, Y).Type == ETileType::Floor) {
        FVector Location(X * TileSize, Y * TileSize,
                         WallHeight * LightHeightRatio);

        APointLight *Light = SpawnLight(
            World, Location, BaseLightIntensity * 0.8f, RoomLightClass);
        if (Light) {
          OutLights.Add(Light);
        }
      }
    }
  }

  UE_LOG(LogTemp, Log, TEXT("DungeonLightingManager: Placed %d cave lights"),
         OutLights.Num());
}

TArray<FIntPoint>
UDungeonLightingManager::FindRoomCenters(const FDungeonGrid &Grid) {
  TArray<FIntPoint> Centers;
  TSet<int32> Visited;

  // Flood fill로 방 영역 찾기
  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      int32 Idx = Y * Grid.Width + X;
      if (Visited.Contains(Idx)) {
        continue;
      }

      if (Grid.GetTile(X, Y).Type == ETileType::Floor) {
        // BFS로 연결된 바닥 타일 찾기
        TArray<FIntPoint> RoomTiles;
        TQueue<FIntPoint> Queue;
        Queue.Enqueue(FIntPoint(X, Y));
        Visited.Add(Idx);

        while (!Queue.IsEmpty()) {
          FIntPoint Current;
          Queue.Dequeue(Current);
          RoomTiles.Add(Current);

          // 4방향 확인
          static const FIntPoint Dirs[] = {{0, -1}, {1, 0}, {0, 1}, {-1, 0}};
          for (const FIntPoint &Dir : Dirs) {
            int32 NX = Current.X + Dir.X;
            int32 NY = Current.Y + Dir.Y;
            int32 NIdx = NY * Grid.Width + NX;

            if (Grid.IsValid(NX, NY) && !Visited.Contains(NIdx) &&
                Grid.GetTile(NX, NY).Type == ETileType::Floor) {
              Visited.Add(NIdx);
              Queue.Enqueue(FIntPoint(NX, NY));
            }
          }
        }

        // 방이 충분히 크면 중심 계산
        if (RoomTiles.Num() >= 10) // 최소 10타일
        {
          int32 SumX = 0, SumY = 0;
          for (const FIntPoint &Tile : RoomTiles) {
            SumX += Tile.X;
            SumY += Tile.Y;
          }
          Centers.Add(
              FIntPoint(SumX / RoomTiles.Num(), SumY / RoomTiles.Num()));
        }
      }
    }
  }

  return Centers;
}

TArray<FIntPoint>
UDungeonLightingManager::FindCorridorTiles(const FDungeonGrid &Grid) {
  TArray<FIntPoint> Corridors;

  for (int32 Y = 0; Y < Grid.Height; Y++) {
    for (int32 X = 0; X < Grid.Width; X++) {
      if (Grid.GetTile(X, Y).Type == ETileType::Corridor) {
        Corridors.Add(FIntPoint(X, Y));
      }
    }
  }

  return Corridors;
}

float UDungeonLightingManager::CalculateLightIntensity(int32 RoomSize) {
  // 방 크기에 비례하여 강도 증가
  return BaseLightIntensity *
         FMath::Clamp(1.0f + RoomSize / 100.0f, 1.0f, 3.0f);
}

APointLight *
UDungeonLightingManager::SpawnLight(UWorld *World, const FVector &Location,
                                    float Intensity,
                                    TSubclassOf<APointLight> LightClass) {
  if (!World) {
    return nullptr;
  }

  UClass* ClassToSpawn = LightClass ? LightClass.Get() : APointLight::StaticClass();

  FActorSpawnParameters SpawnParams;
  SpawnParams.SpawnCollisionHandlingOverride =
      ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

  APointLight *Light = World->SpawnActor<APointLight>(
      ClassToSpawn, Location, FRotator::ZeroRotator, SpawnParams);

  if (Light) {
    Light->PointLightComponent->SetIntensity(Intensity);
    Light->PointLightComponent->SetAttenuationRadius(LightAttenuationRadius);
    Light->SetMobility(EComponentMobility::Movable);
  }

  return Light;
}
