#pragma once

#include "CoreDungeonGrid.h"
#include "CoreInterfaces.h"
#include "CoreTypes.h"
#include <map>
#include <vector>


namespace DungeonCore {

// 프롭 유형 정의
enum class ECorePropType : uint8_t {
  None,
  Chest,  // 상자
  Barrel, // 통
  Torch,  // 횃불
  Statue, // 동상
  Trap    // 함정
};

// 프롭 데이터
struct CorePropData {
  int32_t ID;
  ECorePropType Type;
  int32_t X, Y;
};

// 프롭 생성 설정
struct CorePropConfig {
  ECorePropType Type;
  float Probability;         // 생성 확률
  int32_t Size;              // 크기 (타일 단위)
  float MinDistance;         // 다른 프롭과의 최소 거리
  float AvoidSameTypeRadius; // 동일 유형 간 회피 반경
};

// 핵심 오브젝트 배치 관리자
class CoreObjectPlacer {
public:
  // 몬스터 구역 계산 (공간 해싱 사용)
  static void CalculateMonsterZones(CoreDungeonGrid &Grid, float CheckRadius,
                                    int32_t MinOpenSpace,
                                    ILogger *Logger = nullptr);

  // 프롭 생성 (다양성 보장)
  static std::vector<CorePropData>
  GenerateProps(const CoreDungeonGrid &Grid,
                const std::vector<CorePropConfig> &Configs, IRandom &Random,
                bool bEnforceDiversity, ILogger *Logger = nullptr);

private:
  // 프롭 다양성 체크 (동일 유형이 너무 가까이 있는지 확인)
  static bool CheckPropDiversity(
      float X, float Y, ECorePropType Type,
      const std::map<ECorePropType, std::vector<CorePropData>> &PropsByType,
      float Radius);
};

} // namespace DungeonCore
