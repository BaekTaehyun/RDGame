#pragma once

#include "CoreMinimal.h"
#include "DungeonGrid.h"
#include "ProceduralMeshComponent.h"
#include "DungeonMeshGenerator.generated.h"

/**
 * Marching Squares 기반 Cellular Automata 던전 메시 생성기
 * 동굴 같은 유기적인 형태의 벽을 프로시저럴 메시로 생성
 */
UCLASS(BlueprintType)
class DUNGEONGENERATOR_API UDungeonMeshGenerator : public UObject {
  GENERATED_BODY()

public:
  UDungeonMeshGenerator();

  // 동굴 벽 머티리얼
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
  UMaterialInterface *CaveWallMaterial;

  // 동굴 바닥 머티리얼
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
  UMaterialInterface *CaveFloorMaterial;

  // 동굴 천장 머티리얼
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Materials")
  UMaterialInterface *CaveCeilingMaterial;

  // 타일 크기
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float TileSize = 100.0f;

  // 벽 높이
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float WallHeight = 300.0f;

  // 곡선 부드러움 (0~1)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings",
            meta = (ClampMin = "0", ClampMax = "1"))
  float CurveSmoothing = 0.5f;

  // 천장 생성 여부
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  bool bGenerateCeiling = true;

  // 바닥 생성 여부
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  bool bGenerateFloor = true;

  /**
   * Cellular Automata 던전의 동굴 벽 생성
   * @param Grid - 던전 그리드
   * @param WallMeshComponent - 벽 프로시저럴 메시 컴포넌트
   * @param FloorMeshComponent - 바닥 메시 컴포넌트 (선택)
   * @param CeilingMeshComponent - 천장 메시 컴포넌트 (선택)
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  void
  GenerateCaveWalls(const FDungeonGrid &Grid,
                    UProceduralMeshComponent *WallMeshComponent,
                    UProceduralMeshComponent *FloorMeshComponent = nullptr,
                    UProceduralMeshComponent *CeilingMeshComponent = nullptr);

  /**
   * Marching Squares 구성 계산 (4 corners)
   * @return 0~15 구성 값
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Rendering")
  uint8 GetMarchingSquareConfig(int32 X, int32 Y,
                                const FDungeonGrid &Grid) const;

private:
  // Marching Squares 지오메트리 생성
  void GenerateMarchingSquareGeometry(TArray<FVector> &Vertices,
                                      TArray<int32> &Triangles,
                                      TArray<FVector2D> &UVs,
                                      TArray<FVector> &Normals, int32 X,
                                      int32 Y, uint8 Config);

  // 노멀 스무딩 적용
  void ApplySmoothNormals(TArray<FVector> &Normals,
                          const TArray<FVector> &Vertices,
                          const TArray<int32> &Triangles);

  // 바닥 메시 생성
  void GenerateFloorMesh(const FDungeonGrid &Grid,
                         UProceduralMeshComponent *FloorMeshComponent);

  // 천장 메시 생성
  void GenerateCeilingMesh(const FDungeonGrid &Grid,
                           UProceduralMeshComponent *CeilingMeshComponent);

  // 타일이 벽인지 확인
  bool IsWall(int32 X, int32 Y, const FDungeonGrid &Grid) const;

  // 보간된 정점 위치 계산
  FVector GetInterpolatedVertex(int32 X, int32 Y, bool bVertical,
                                float T) const;
};
