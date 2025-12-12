#pragma once

#include "CoreTypes.h"
#include <cstdint>
#include <vector>

namespace DungeonCore {

// 핵심 던전 그리드 데이터 구조 (std::vector 사용)
class CoreDungeonGrid {
public:
  int32_t Width;
  int32_t Height;
  std::vector<FCoreTile> Tiles;

  CoreDungeonGrid() : Width(0), Height(0) {}
  CoreDungeonGrid(int32_t InWidth, int32_t InHeight,
                  ETileType InitialType = ETileType::Wall) {
    Init(InWidth, InHeight, InitialType);
  }

  // 그리드 초기화
  void Init(int32_t InWidth, int32_t InHeight,
            ETileType InitialType = ETileType::Wall) {
    Width = InWidth;
    Height = InHeight;
    Tiles.resize(Width * Height);

    for (int32_t Y = 0; Y < Height; Y++) {
      for (int32_t X = 0; X < Width; X++) {
        int32_t Index = Y * Width + X;
        Tiles[Index].X = X;
        Tiles[Index].Y = Y;
        Tiles[Index].Type = InitialType;
        Tiles[Index].RoomID = -1;
        Tiles[Index].StairTargetFloor = -1;
      }
    }
  }

  // 타일 접근 (참조 반환)
  FCoreTile &GetTile(int32_t X, int32_t Y) { return Tiles[Y * Width + X]; }

  const FCoreTile &GetTile(int32_t X, int32_t Y) const {
    return Tiles[Y * Width + X];
  }

  // 좌표 유효성 검사
  bool IsValid(int32_t X, int32_t Y) const {
    return X >= 0 && X < Width && Y >= 0 && Y < Height;
  }
};

} // namespace DungeonCore
