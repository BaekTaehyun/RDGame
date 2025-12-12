#include "CoreBSPGenerator.h"
#include <algorithm>
#include <cmath>

namespace DungeonCore {

void CoreBSPGenerator::Generate(CoreDungeonGrid &Grid, IRandom &Random,
                                ILogger *Logger) {
  if (Grid.Width < MinNodeSize * 2 || Grid.Height < MinNodeSize * 2) {
    if (Logger)
      Logger->LogError("Grid too small for BSP generation");
    return;
  }

  // 루트 노드 생성
  auto RootNode = std::make_shared<CoreBSPNode>(0, 0, Grid.Width, Grid.Height);

  // 재귀적 분할
  SplitNode(RootNode, Random, Logger);

  // 방 생성
  CreateRooms(RootNode, Grid, Random);

  // 방 연결 (복도 생성)
  ConnectRooms(RootNode, Grid, Random);
}

void CoreBSPGenerator::SplitNode(std::shared_ptr<CoreBSPNode> Node,
                                 IRandom &Random, ILogger *Logger) {
  if (!Node)
    return;

  // 더 이상 분할할 수 없으면 중단
  if (Node->Width < MinNodeSize * 2 && Node->Height < MinNodeSize * 2)
    return;

  bool bSplitH = Random.GetFraction() > 0.5f;
  const float AspectRatioThreshold = 1.25f;

  // 비율에 따라 분할 방향 결정
  if (Node->Width > Node->Height &&
      (float)Node->Width / Node->Height >= AspectRatioThreshold) {
    bSplitH = false; // 수직 분할
  } else if (Node->Height > Node->Width &&
             (float)Node->Height / Node->Width >= AspectRatioThreshold) {
    bSplitH = true; // 수평 분할
  }

  int32_t Max = (bSplitH ? Node->Height : Node->Width) - MinNodeSize;
  if (Max <= MinNodeSize)
    return;

  int32_t SplitPos = Random.RandRange(MinNodeSize, Max);

  if (bSplitH) {
    Node->Left =
        std::make_shared<CoreBSPNode>(Node->X, Node->Y, Node->Width, SplitPos);
    Node->Right = std::make_shared<CoreBSPNode>(
        Node->X, Node->Y + SplitPos, Node->Width, Node->Height - SplitPos);
  } else {
    Node->Left =
        std::make_shared<CoreBSPNode>(Node->X, Node->Y, SplitPos, Node->Height);
    Node->Right = std::make_shared<CoreBSPNode>(
        Node->X + SplitPos, Node->Y, Node->Width - SplitPos, Node->Height);
  }
  Node->bIsLeaf = false;

  SplitNode(Node->Left, Random, Logger);
  SplitNode(Node->Right, Random, Logger);
}

void CoreBSPGenerator::CreateRooms(std::shared_ptr<CoreBSPNode> Node,
                                   CoreDungeonGrid &Grid, IRandom &Random) {
  if (!Node)
    return;

  if (Node->bIsLeaf) {
    // 리프 노드 내부에 랜덤한 크기의 방 생성
    int32_t RoomWidth = Random.RandRange(MinRoomSize, Node->Width - 2);
    int32_t RoomHeight = Random.RandRange(MinRoomSize, Node->Height - 2);
    int32_t RoomX = Random.RandRange(1, Node->Width - RoomWidth - 1);
    int32_t RoomY = Random.RandRange(1, Node->Height - RoomHeight - 1);

    Node->RoomX = Node->X + RoomX;
    Node->RoomY = Node->Y + RoomY;
    Node->RoomWidth = RoomWidth;
    Node->RoomHeight = RoomHeight;

    for (int32_t Y = Node->RoomY; Y < Node->RoomY + RoomHeight; Y++) {
      for (int32_t X = Node->RoomX; X < Node->RoomX + RoomWidth; X++) {
        if (Grid.IsValid(X, Y)) {
          Grid.GetTile(X, Y).Type = ETileType::Floor;
        }
      }
    }
  } else {
    CreateRooms(Node->Left, Grid, Random);
    CreateRooms(Node->Right, Grid, Random);
  }
}

void CoreBSPGenerator::ConnectRooms(std::shared_ptr<CoreBSPNode> Node,
                                    CoreDungeonGrid &Grid, IRandom &Random) {
  if (!Node || Node->bIsLeaf)
    return;

  ConnectRooms(Node->Left, Grid, Random);
  ConnectRooms(Node->Right, Grid, Random);

  // 간단한 연결 로직 (중심점 연결)
  // 실제 구현에서는 A* 알고리즘 등을 사용하여 더 자연스러운 경로를 만들 수
  // 있습니다.

  int32_t X1 = Node->Left->X + Node->Left->Width / 2;
  int32_t Y1 = Node->Left->Y + Node->Left->Height / 2;
  int32_t X2 = Node->Right->X + Node->Right->Width / 2;
  int32_t Y2 = Node->Right->Y + Node->Right->Height / 2;

  CreateCorridor(Grid, X1, Y1, X2, Y2);
}

void CoreBSPGenerator::CreateCorridor(CoreDungeonGrid &Grid, int32_t X1,
                                      int32_t Y1, int32_t X2, int32_t Y2) {
  int32_t X = X1;
  int32_t Y = Y1;

  // 복도 폭 계산 (중앙 기준 양쪽으로 확장)
  int32_t HalfWidth = CorridorWidth / 2;

  // 수평 이동 (X 방향)
  while (X != X2) {
    // 복도 폭만큼 타일 생성
    for (int32_t Offset = -HalfWidth; Offset < CorridorWidth - HalfWidth; Offset++) {
      if (Grid.IsValid(X, Y + Offset)) {
        Grid.GetTile(X, Y + Offset).Type = ETileType::Corridor;
      }
    }
    X += (X2 > X) ? 1 : -1;
  }
  
  // 수직 이동 (Y 방향)
  while (Y != Y2) {
    // 복도 폭만큼 타일 생성
    for (int32_t Offset = -HalfWidth; Offset < CorridorWidth - HalfWidth; Offset++) {
      if (Grid.IsValid(X + Offset, Y)) {
        Grid.GetTile(X + Offset, Y).Type = ETileType::Corridor;
      }
    }
    Y += (Y2 > Y) ? 1 : -1;
  }
}

// 다층 던전 구현
CoreMultiFloorDungeon
CoreBSPGenerator::GenerateMultiFloor(const CoreMultiFloorConfig &Config,
                                     IRandom &Random, ILogger *Logger) {
  CoreMultiFloorDungeon MultiFloor;
  MultiFloor.NumFloors = Config.NumFloors;
  MultiFloor.Floors.resize(Config.NumFloors);

  // 순차적 생성 (Core에서는 단순성을 위해 순차 처리, 병렬 처리는 호출자에게
  // 위임 가능)
  for (int32_t i = 0; i < Config.NumFloors; i++) {
    CoreDungeonGrid Grid(Config.Width, Config.Height, ETileType::Wall);
    CoreBSPGenerator Generator; // 기본 설정 사용 또는 Config 전달
    Generator.Generate(Grid, Random, Logger);
    MultiFloor.Floors[i] = Grid;
  }

  // 수직 정렬 강제
  if (Config.bEnforceVerticalAlignment) {
    EnforceVerticalAlignment(MultiFloor, Logger);
  }

  // 계단 배치
  MultiFloor.Stairs = PlaceStairs(MultiFloor, Config, Random, Logger);
  MarkStairTiles(MultiFloor);

  return MultiFloor;
}

void CoreBSPGenerator::EnforceVerticalAlignment(
    CoreMultiFloorDungeon &MultiFloor, ILogger *Logger) {
  for (int32_t Floor = MultiFloor.NumFloors - 1; Floor > 0; Floor--) {
    auto &CurrentFloor = MultiFloor.Floors[Floor];
    auto &BelowFloor = MultiFloor.Floors[Floor - 1];

    for (int32_t Y = 0; Y < CurrentFloor.Height; Y++) {
      for (int32_t X = 0; X < CurrentFloor.Width; X++) {
        // 아래층이 벽이면 현재 층도 벽이어야 함 (플레이어 추락 방지)
        if (BelowFloor.GetTile(X, Y).Type == ETileType::Wall &&
            CurrentFloor.GetTile(X, Y).Type != ETileType::Wall) {
          CurrentFloor.GetTile(X, Y).Type = ETileType::Wall;
        }
      }
    }
  }
}

std::vector<CoreStairPosition>
CoreBSPGenerator::PlaceStairs(CoreMultiFloorDungeon &MultiFloor,
                              const CoreMultiFloorConfig &Config,
                              IRandom &Random, ILogger *Logger) {
  std::vector<CoreStairPosition> AllStairs;
  float MinDistSq = Config.MinStairDistance * Config.MinStairDistance;

  for (int32_t Floor = 0; Floor < MultiFloor.NumFloors - 1; Floor++) {
    std::vector<FIntPoint> ValidPositions;
    for (int32_t Y = 0; Y < MultiFloor.Floors[Floor].Height; Y++) {
      for (int32_t X = 0; X < MultiFloor.Floors[Floor].Width; X++) {
        // 두 층 모두 바닥인 곳만 계단 설치 가능
        bool bCurrentFloor =
            MultiFloor.Floors[Floor].GetTile(X, Y).Type != ETileType::Wall;
        bool bNextFloor =
            MultiFloor.Floors[Floor + 1].GetTile(X, Y).Type != ETileType::Wall;

        if (bCurrentFloor && bNextFloor) {
          ValidPositions.push_back(FIntPoint(X, Y));
        }
      }
    }

    if (ValidPositions.empty())
      continue;

    // 셔플
    for (size_t i = ValidPositions.size() - 1; i > 0; i--) {
      size_t j = Random.RandRange(0, (int32_t)i);
      std::swap(ValidPositions[i], ValidPositions[j]);
    }

    int32_t StairsPlaced = 0;
    for (const auto &Pos : ValidPositions) {
      if (StairsPlaced >= Config.StairsPerFloor)
        break;

      bool bTooClose = false;
      for (const auto &Stair : AllStairs) {
        if (Stair.FloorIndex == Floor) {
          float DistSq =
              std::pow(Pos.X - Stair.X, 2) + std::pow(Pos.Y - Stair.Y, 2);
          if (DistSq < MinDistSq) {
            bTooClose = true;
            break;
          }
        }
      }

      if (!bTooClose) {
        AllStairs.push_back({Floor, Pos.X, Pos.Y, Floor + 1});
        StairsPlaced++;
      }
    }
  }
  return AllStairs;
}

void CoreBSPGenerator::MarkStairTiles(CoreMultiFloorDungeon &MultiFloor) {
  for (const auto &Stair : MultiFloor.Stairs) {
    if (Stair.FloorIndex >= 0 && Stair.FloorIndex < MultiFloor.NumFloors) {
      MultiFloor.Floors[Stair.FloorIndex].GetTile(Stair.X, Stair.Y).Type =
          ETileType::Stair;
      MultiFloor.Floors[Stair.FloorIndex]
          .GetTile(Stair.X, Stair.Y)
          .StairTargetFloor = Stair.TargetFloor;
    }
    if (Stair.TargetFloor >= 0 && Stair.TargetFloor < MultiFloor.NumFloors) {
      MultiFloor.Floors[Stair.TargetFloor].GetTile(Stair.X, Stair.Y).Type =
          ETileType::Stair;
      MultiFloor.Floors[Stair.TargetFloor]
          .GetTile(Stair.X, Stair.Y)
          .StairTargetFloor = Stair.FloorIndex;
    }
  }
}

} // namespace DungeonCore
