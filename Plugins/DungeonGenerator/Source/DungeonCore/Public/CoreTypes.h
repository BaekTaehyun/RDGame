#pragma once

#include <cstdint>

namespace DungeonCore {

// 타일 유형 정의 (플랫폼 독립적)
enum class ETileType : uint8_t {
  None,     // 없음
  Floor,    // 바닥
  Wall,     // 벽
  Corridor, // 복도
  Door,     // 문
  Stair,    // 계단
  Debug     // 디버그용
};

// 정수형 2D 좌표 구조체
struct FIntPoint {
  int32_t X;
  int32_t Y;

  FIntPoint() : X(0), Y(0) {}
  FIntPoint(int32_t InX, int32_t InY) : X(InX), Y(InY) {}

  bool operator==(const FIntPoint &Other) const {
    return X == Other.X && Y == Other.Y;
  }
};

// 핵심 타일 데이터 구조체
struct FCoreTile {
  int32_t X;
  int32_t Y;
  ETileType Type;
  int32_t RoomID;           // 방 ID (-1이면 방 아님)
  float MonsterSuitability; // 몬스터 배치 적합도 (0.0 ~ 1.0)
  int32_t StairTargetFloor; // 계단 연결 층 인덱스 (-1이면 없음)

  FCoreTile()
      : X(0), Y(0), Type(ETileType::None), RoomID(-1), MonsterSuitability(0.0f),
        StairTargetFloor(-1) {}
};

} // namespace DungeonCore
