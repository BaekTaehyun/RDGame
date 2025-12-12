#pragma once

#include "CoreMinimal.h"
#include "DungeonGrid.h"
#include "NavMesh/RecastNavMesh.h"

#include "DungeonNavigationBuilder.generated.h"

/**
 * 던전 NavMesh 및 콜리전 자동 생성 관리자
 */
UCLASS(BlueprintType)
class DUNGEONGENERATOR_API UDungeonNavigationBuilder : public UObject {
  GENERATED_BODY()

public:
  UDungeonNavigationBuilder();

  // 타일 크기
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float TileSize = 100.0f;

  // 벽 높이
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float WallHeight = 300.0f;

  // NavMesh 자동 생성 여부
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  bool bAutoGenerateNavMesh = true;

  /**
   * NavMesh 빌드
   * @param Grid - 던전 그리드
   * @param World - 월드 컨텍스트
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Navigation")
  void BuildNavMesh(const FDungeonGrid &Grid, UWorld *World);

  /**
   * NavMesh Bounds 자동 계산
   * @param Grid - 던전 그리드
   * @param Offset - 네비게이션 바운즈 오프셋
   * @return NavMesh에 적용할 Bounds
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Navigation")
  FBox CalculateNavBounds(const FDungeonGrid &Grid, FVector Offset = FVector::ZeroVector) const;

  // NavMesh 생성 트리거
  void TriggerNavMeshRebuild(UWorld *World, const FBox &Bounds);
};
