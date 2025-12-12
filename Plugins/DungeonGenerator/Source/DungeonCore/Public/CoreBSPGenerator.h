#pragma once

#include "CoreDungeonGrid.h"
#include "CoreInterfaces.h"
#include "CoreTypes.h"
#include <memory>
#include <vector>

namespace DungeonCore {

// BSP 트리 노드 구조체
struct CoreBSPNode {
  int32_t X, Y, Width, Height;
  std::shared_ptr<CoreBSPNode> Left;
  std::shared_ptr<CoreBSPNode> Right;
  int32_t RoomX, RoomY, RoomWidth, RoomHeight;
  bool bIsLeaf = true;

  CoreBSPNode(int32_t InX, int32_t InY, int32_t InWidth, int32_t InHeight)
      : X(InX), Y(InY), Width(InWidth), Height(InHeight), RoomX(0), RoomY(0),
        RoomWidth(0), RoomHeight(0) {}
};

// 다층 던전 설정 구조체
struct CoreMultiFloorConfig {
  int32_t NumFloors = 1; // 층 수
  int32_t Width = 50;    // 너비
  int32_t Height = 50;   // 높이
  bool bEnforceVerticalAlignment =
      true; // 수직 정렬 강제 여부 (위층 바닥 아래는 반드시 바닥/벽)
  int32_t StairsPerFloor = 2;     // 층당 계단 수
  float MinStairDistance = 15.0f; // 계단 간 최소 거리
};

// 계단 위치 정보
struct CoreStairPosition {
  int32_t FloorIndex;
  int32_t X, Y;
  int32_t TargetFloor;
};

// 다층 던전 결과 데이터
struct CoreMultiFloorDungeon {
  std::vector<CoreDungeonGrid> Floors;
  int32_t NumFloors;
  std::vector<CoreStairPosition> Stairs;
};

// 핵심 BSP 던전 생성기
class DUNGEONCORE_API CoreBSPGenerator {
public:
  int32_t MinNodeSize = 10; // 노드 최소 크기
  int32_t MinRoomSize = 6;  // 방 최소 크기
  float SplitRatio = 0.45f; // 분할 비율
  int32_t CorridorWidth = 3; // 복도 폭 (1 이상, 3 권장)

  // 단일 층 생성
  void Generate(CoreDungeonGrid &Grid, IRandom &Random,
                ILogger *Logger = nullptr);

  // 다층 던전 생성 (순차적)
  static CoreMultiFloorDungeon
  GenerateMultiFloor(const CoreMultiFloorConfig &Config, IRandom &Random,
                     ILogger *Logger = nullptr);

  // 헬퍼 함수들 (커스텀 파이프라인 구성을 위해 공개)
  static void EnforceVerticalAlignment(CoreMultiFloorDungeon &MultiFloor,
                                       ILogger *Logger);
  static std::vector<CoreStairPosition>
  PlaceStairs(CoreMultiFloorDungeon &MultiFloor,
              const CoreMultiFloorConfig &Config, IRandom &Random,
              ILogger *Logger);
  static void MarkStairTiles(CoreMultiFloorDungeon &MultiFloor);

private:
  void SplitNode(std::shared_ptr<CoreBSPNode> Node, IRandom &Random,
                 ILogger *Logger);
  void CreateRooms(std::shared_ptr<CoreBSPNode> Node, CoreDungeonGrid &Grid,
                   IRandom &Random);
  void ConnectRooms(std::shared_ptr<CoreBSPNode> Node, CoreDungeonGrid &Grid,
                    IRandom &Random);
  void CreateCorridor(CoreDungeonGrid &Grid, int32_t X1, int32_t Y1,
                      int32_t X2, int32_t Y2);
};

} // namespace DungeonCore
