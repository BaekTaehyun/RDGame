#pragma once

#include "Algorithms/DungeonAlgorithm.h"
#include "CoreMinimal.h"
#include "Data/DungeonPresetData.h"
#include "PresetAssemblyGenerator.generated.h"

/**
 * 그리드 상에 배치된 모듈 정보
 */
struct FPlacedModule {
  FName ModuleID;
  FIntPoint Position; // 그리드 상의 원점 좌표
  FIntPoint Size;     // 모듈 크기 (회전 고려 시 변할 수 있음)
                      // int32 Rotation; // 0, 90, 180, 270 (V1에서는 생략)
};

/**
 * 확장 대기 중인 소켓 정보 (World Space)
 */
struct FOpenSocketInfo {
  FModuleSocket SocketData;
  FIntPoint WorldPosition; // 이 소켓의 그리드 절대 좌표
};

/**
 * 디아블로 2 스타일: 프리셋 모듈 조립 알고리즘
 * 미리 정의된 방/복도 조각을 소켓 규칙에 맞춰 이어 붙입니다.
 */
UCLASS()
class DUNGEONGENERATOR_API UPresetAssemblyGenerator : public UDungeonAlgorithm {
  GENERATED_BODY()

public:
  virtual void Generate(FDungeonGrid &Grid, FRandomStream &Stream) override;

  // --- Configuration (Injected by Builder) ---

  UPROPERTY(Transient)
  UPresetModuleDatabase *ModuleDatabase;

  int32 MaxRoomCount = 20;

private:
  // 배치된 모듈 목록
  TArray<FPlacedModule> PlacedModules;

  // --- Helper Functions ---

  /** 루트(Start) 모듈 배치 시도 */
  bool PlaceStartModule(FDungeonGrid &Grid, FRandomStream &Stream,
                        TArray<FOpenSocketInfo> &OutOpenSockets);

  /** 특정 소켓에 맞는 다음 모듈을 찾아 배치 시도 */
  bool TryPlaceNextModule(FDungeonGrid &Grid, FRandomStream &Stream,
                          const FOpenSocketInfo &TargetSocket,
                          TArray<FOpenSocketInfo> &OutOpenSockets);

  /** 모듈이 그리드 내에 있고 다른 모듈과 겹치지 않는지 확인 */
  bool CanPlaceModule(const FDungeonGrid &Grid, const FModuleData &Module,
                      FIntPoint Position);

  /** 모듈을 그리드에 기록하고 새 소켓들을 반환 */
  void StampModuleToGrid(FDungeonGrid &Grid, const FModuleData &Module,
                         FIntPoint Position,
                         TArray<FOpenSocketInfo> &OutNewSockets);

  /** 소켓 방향의 역방향 구하기 (North <-> South) */
  EModuleSocketDirection GetOppositeDirection(EModuleSocketDirection Dir);

  /** 방향에 따른 좌표 오프셋 구하기 */
  FIntPoint GetDirectionOffset(EModuleSocketDirection Dir);
};
