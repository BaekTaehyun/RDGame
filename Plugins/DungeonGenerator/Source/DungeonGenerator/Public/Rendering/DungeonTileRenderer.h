#pragma once

#include "Components/InstancedStaticMeshComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "CoreMinimal.h"
#include "DungeonGrid.h"
#include "DungeonTileRenderer.generated.h"

/**
 * Bitmasking 기반 BSP 던전 타일 렌더러
 * 4방향 이웃 검사를 통해 적절한 벽 메시를 자동 선택하고 배치
 */
UCLASS(BlueprintType)
class DUNGEONGENERATOR_API UDungeonTileRenderer : public UObject {
  GENERATED_BODY()

public:
  UDungeonTileRenderer();

  // 벽 타일 비트마스크 테이블 (0~15)
  // 0000 = 고립된 벽
  // 0001 = 북쪽 연결
  // 0010 = 동쪽 연결
  // 0011 = 북동 코너
  // ... 등
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Wall Meshes")
  TMap<uint8, UStaticMesh *> WallMeshTable;

  // 천장 메시
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ceiling")
  UStaticMesh *CeilingMesh;

  // 바닥 메시
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Floor")
  UStaticMesh *FloorMesh;

  // 타일 크기 (언리얼 유닛)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float TileSize = 100.0f;

  // 벽 높이
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float WallHeight = 300.0f;

  // 벽 피봇 오프셋
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  FVector WallPivotOffset;

  // 바닥 피봇 오프셋
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  FVector FloorPivotOffset;

  // 천장 피봇 오프셋
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  FVector CeilingPivotOffset;

  // 천장 생성 여부
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  bool bGenerateCeiling = true;

  // 바닥 생성 여부
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  bool bGenerateFloor = true;

  // 벽 아래에도 바닥 생성 (전체 바닥 옵션)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  bool bGenerateFloorUnderWalls = true;

  // LOD 레벨 사용 여부
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
  bool bUseLOD = true;

  // LOD 거리 배열 [LOD0, LOD1, LOD2]
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "LOD")
  TArray<float> LODDistances = {5000.0f, 10000.0f, 20000.0f};

  // 동적 머티리얼 사용
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
  bool bUseDynamicMaterials = false;

  // 습기 강도 (0-1)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials",
            meta = (ClampMin = "0", ClampMax = "1"))
  float WetnessIntensity = 0.0f;

  // 이끼 강도 (0-1)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials",
            meta = (ClampMin = "0", ClampMax = "1"))
  float MossIntensity = 0.0f;

  // --- 청크 기반 컬링 설정 ---
  // 청크 사용 여부 (대형 던전에 권장)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunking")
  bool bUseChunking = true;

  // 청크 크기 (타일 단위, 예: 10 = 10x10 타일 = 1000x1000 유닛)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Chunking",
            meta = (ClampMin = "5", ClampMax = "50"))
  int32 ChunkSize = 10;

  // --- 디버그 설정 ---
  // 그리드 디버그 파일 출력 여부
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Debug")
  bool bDebugOutputGrid = false;

  // --- 메시 머징 설정 (Phase 3) ---
  // 메시 머징 활성화 여부 (HISM을 단일 DynamicMesh로 병합)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merging")
  bool bEnableMeshMerging = false;

  // 머징 후 원본 HISM 제거 여부
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Mesh Merging")
  bool bRemoveOriginalAfterMerge = true;

  /**
   * BSP 던전의 벽 타일 생성 (단일 ISMC 버전 - 하위 호환)
   * @param Grid - 던전 그리드
   * @param WallISMC - 벽 인스턴스 컴포넌트
   * @param CeilingISMC - 천장 인스턴스 컴포넌트 (선택)
   * @param FloorISMC - 바닥 인스턴스 컴포넌트 (선택)
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  void GenerateBSPTiles(const FDungeonGrid &Grid,
                        UInstancedStaticMeshComponent *WallISMC,
                        UInstancedStaticMeshComponent *CeilingISMC = nullptr,
                        UInstancedStaticMeshComponent *FloorISMC = nullptr);

  /**
   * BSP 던전의 벽 타일 생성 (다중 메시 버전 - HISM)
   * WallMeshTable의 각 메시별로 별도의 HISM을 동적 생성
   * @param Grid - 던전 그리드
   * @param OwnerActor - HISM을 생성할 Actor
   * @param CeilingISMC - 천장 인스턴스 컴포넌트 (선택)
   * @param FloorISMC - 바닥 인스턴스 컴포넌트 (선택)
   * @param OutCreatedHISMs - 생성된 HISM 맵 (Mask -> HISM)
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  void GenerateBSPTilesMultiMesh(const FDungeonGrid &Grid,
                                  AActor* OwnerActor,
                                  UHierarchicalInstancedStaticMeshComponent *CeilingHISM,
                                  UHierarchicalInstancedStaticMeshComponent *FloorHISM,
                                  TArray<UHierarchicalInstancedStaticMeshComponent*>& OutCreatedHISMs);

  /**
   * 테마 에셋의 설정을 렌더러에 적용
   * @param Theme - 적용할 테마 에셋
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  void ApplyTheme(const class UDungeonThemeAsset* Theme);

  /**
   * 특정 위치의 벽 비트마스크 계산 (4방향)
   * @return 0~15 비트마스크 값
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  uint8 GetWallBitmask(int32 X, int32 Y, const FDungeonGrid &Grid) const;

  /**
   * 8방향 비트마스크 계산 (더 정교한 버전)
   * @return 0~255 비트마스크 값
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  uint8 GetWallBitmask8Dir(int32 X, int32 Y, const FDungeonGrid &Grid) const;

private:
  // 타일이 벽인지 확인
  bool IsWall(int32 X, int32 Y, const FDungeonGrid &Grid) const;

  // 천장 생성 (HISM 버전)
  void GenerateCeiling(const FDungeonGrid &Grid,
                       UHierarchicalInstancedStaticMeshComponent *CeilingHISM);

  // 바닥 생성 (HISM 버전)
  void GenerateFloor(const FDungeonGrid &Grid,
                     UHierarchicalInstancedStaticMeshComponent *FloorHISM);
  // LOD 설정 (HISM)
  void SetupLOD(UHierarchicalInstancedStaticMeshComponent* HISM);

  // 다이나믹 머티리얼 적용 (HISM)
  void ApplyDynamicMaterial(UHierarchicalInstancedStaticMeshComponent* HISM);
};