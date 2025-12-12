#include "CoreObjectPlacer.h"
#include <algorithm>
#include <cmath>

namespace DungeonCore {

// 공간 해싱을 위한 그리드 구조체
struct CoreSpatialGrid {
  std::map<int32_t, std::vector<FIntPoint>> Cells;
  int32_t CellSize;
  int32_t GridWidth;

  CoreSpatialGrid(int32_t InCellSize, int32_t InGridWidth)
      : CellSize(InCellSize), GridWidth(InGridWidth) {}

  int32_t GetCellKey(int32_t X, int32_t Y) const {
    return (X / CellSize) * 10000 + (Y / CellSize);
  }

  void AddTile(int32_t X, int32_t Y) {
    Cells[GetCellKey(X, Y)].push_back(FIntPoint(X, Y));
  }

  void QueryRadius(int32_t X, int32_t Y, float Radius,
                   std::vector<FIntPoint> &OutTiles) const {
    int32_t MinCellX = (int32_t)((X - Radius) / CellSize);
    int32_t MaxCellX = (int32_t)((X + Radius) / CellSize);
    int32_t MinCellY = (int32_t)((Y - Radius) / CellSize);
    int32_t MaxCellY = (int32_t)((Y + Radius) / CellSize);

    for (int32_t CY = MinCellY; CY <= MaxCellY; CY++) {
      for (int32_t CX = MinCellX; CX <= MaxCellX; CX++) {
        int32_t Key = CX * 10000 + CY;
        auto It = Cells.find(Key);
        if (It != Cells.end()) {
          OutTiles.insert(OutTiles.end(), It->second.begin(), It->second.end());
        }
      }
    }
  }
};

void CoreObjectPlacer::CalculateMonsterZones(CoreDungeonGrid &Grid,
                                             float CheckRadius,
                                             int32_t MinOpenSpace,
                                             ILogger *Logger) {
  // 공간 해싱 최적화 적용
  CoreSpatialGrid SpatialGrid((int32_t)CheckRadius, Grid.Width);

  for (int32_t Y = 0; Y < Grid.Height; Y++) {
    for (int32_t X = 0; X < Grid.Width; X++) {
      if (Grid.GetTile(X, Y).Type == ETileType::Floor) {
        SpatialGrid.AddTile(X, Y);
      }
    }
  }

  float RadiusSq = CheckRadius * CheckRadius;

  // 순차 루프 (서버용 단순 구현, 필요시 병렬 처리 주입 가능)
  for (int32_t Y = 0; Y < Grid.Height; Y++) {
    for (int32_t X = 0; X < Grid.Width; X++) {
      FCoreTile &Tile = Grid.GetTile(X, Y);
      if (Tile.Type != ETileType::Floor) {
        Tile.MonsterSuitability = 0.0f;
        continue;
      }

      std::vector<FIntPoint> NearbyTiles;
      SpatialGrid.QueryRadius(X, Y, CheckRadius, NearbyTiles);

      int32_t OpenCount = 0;
      for (const auto &Pt : NearbyTiles) {
        float DistSq = std::pow(X - Pt.X, 2) + std::pow(Y - Pt.Y, 2);
        if (DistSq <= RadiusSq) {
          OpenCount++;
        }
      }

      if (!NearbyTiles.empty() && OpenCount >= MinOpenSpace) {
        float Ratio = (float)OpenCount / NearbyTiles.size();
        float Bonus = (float)(OpenCount - MinOpenSpace) / MinOpenSpace;
        float Suitability = Ratio + Bonus * 0.2f;
        Tile.MonsterSuitability =
            Suitability > 1.0f ? 1.0f
                               : (Suitability < 0.0f ? 0.0f : Suitability);
      } else {
        Tile.MonsterSuitability = 0.0f;
      }
    }
  }
}

bool CoreObjectPlacer::CheckPropDiversity(
    float X, float Y, ECorePropType Type,
    const std::map<ECorePropType, std::vector<CorePropData>> &PropsByType,
    float Radius) {
  auto It = PropsByType.find(Type);
  if (It == PropsByType.end())
    return true;

  float RadiusSq = Radius * Radius;
  for (const auto &Prop : It->second) {
    float DistSq =
        std::pow(X - (Prop.X + 0.5f), 2) + std::pow(Y - (Prop.Y + 0.5f), 2);
    if (DistSq < RadiusSq)
      return false;
  }
  return true;
}

std::vector<CorePropData> CoreObjectPlacer::GenerateProps(
    const CoreDungeonGrid &Grid, const std::vector<CorePropConfig> &Configs,
    IRandom &Random, bool bEnforceDiversity, ILogger *Logger) {
  std::vector<CorePropData> Props;
  if (Configs.empty())
    return Props;

  float TotalProb = 0.0f;
  for (const auto &Cfg : Configs)
    TotalProb += Cfg.Probability;

  std::map<ECorePropType, std::vector<CorePropData>> PropsByType;
  int32_t PropIDCounter = 0;

  // 포아송 디스크 샘플링 유사 방식 (단순화됨)
  int32_t MaxPoints = 1000;
  for (int32_t i = 0; i < MaxPoints; i++) {
    int32_t X = Random.RandRange(0, Grid.Width - 1);
    int32_t Y = Random.RandRange(0, Grid.Height - 1);

    if (!Grid.IsValid(X, Y) || Grid.GetTile(X, Y).Type != ETileType::Floor)
      continue;

    // 유형 선택
    float Roll = Random.GetFraction() * TotalProb;
    float Cumulative = 0.0f;
    const CorePropConfig *SelectedConfig = &Configs[0];
    for (const auto &Cfg : Configs) {
      Cumulative += Cfg.Probability;
      if (Roll <= Cumulative) {
        SelectedConfig = &Cfg;
        break;
      }
    }

    // 다양성 체크
    if (bEnforceDiversity) {
      if (!CheckPropDiversity(X + 0.5f, Y + 0.5f, SelectedConfig->Type,
                              PropsByType,
                              SelectedConfig->AvoidSameTypeRadius)) {
        continue;
      }
    }

    CorePropData NewProp = {PropIDCounter++, SelectedConfig->Type, X, Y};
    Props.push_back(NewProp);
    PropsByType[NewProp.Type].push_back(NewProp);
  }

  return Props;
}

} // namespace DungeonCore
