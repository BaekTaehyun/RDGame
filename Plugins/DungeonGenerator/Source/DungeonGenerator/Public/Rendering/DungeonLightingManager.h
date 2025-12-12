#pragma once

#include "CoreMinimal.h"
#include "DungeonGrid.h"
#include "Engine/PointLight.h"
#include "DungeonLightingManager.generated.h"



/**
 * 던전 자동 조명 배치 관리자
 * 방, 복도, 동굴 영역에 자동으로 조명 배치
 */
UCLASS(BlueprintType)
class DUNGEONGENERATOR_API UDungeonLightingManager : public UObject {
  GENERATED_BODY()

public:
  UDungeonLightingManager();

  // 방 조명 클래스
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Types")
  TSubclassOf<APointLight> RoomLightClass;

  // 복도 조명 클래스
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Light Types")
  TSubclassOf<APointLight> CorridorLightClass;

  // 복도 조명 간격 (언리얼 유닛)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float CorridorLightSpacing = 500.0f;

  // 기본 조명 강도
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float BaseLightIntensity = 3000.0f;

  // 조명 감쇠 반경
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float LightAttenuationRadius = 1000.0f;

  // 조명 높이 (벽 높이 대비 비율)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float LightHeightRatio = 0.7f;

  // 타일 크기
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float TileSize = 100.0f;

  // 벽 높이
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Settings")
  float WallHeight = 300.0f;

  /**
   * 던전 전체 조명 자동 배치
   * @param Grid - 던전 그리드
   * @param World - 월드 컨텍스트
   * @param OutLights - 생성된 조명 액터들
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Lighting")
  void PlaceDungeonLights(const FDungeonGrid &Grid, UWorld *World,
                          TArray<APointLight *> &OutLights);

  /**
   * 방 중심에 조명 배치
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Lighting")
  void PlaceRoomLights(const FDungeonGrid &Grid, UWorld *World,
                       TArray<APointLight *> &OutLights);

  /**
   * 복도에 간격을 두고 조명 배치
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Lighting")
  void PlaceCorridorLights(const FDungeonGrid &Grid, UWorld *World,
                           TArray<APointLight *> &OutLights);

  /**
   * 동굴 영역에 조명 배치 (CA 던전용)
   */
  UFUNCTION(BlueprintCallable, Category = "Dungeon Lighting")
  void PlaceCaveLights(const FDungeonGrid &Grid, UWorld *World,
                       TArray<APointLight *> &OutLights);

private:
  // 방 중심 찾기 (flood fill)
  TArray<FIntPoint> FindRoomCenters(const FDungeonGrid &Grid);

  // 복도 타일 찾기
  TArray<FIntPoint> FindCorridorTiles(const FDungeonGrid &Grid);

  // 공간 크기 기반 조명 강도 계산
  float CalculateLightIntensity(int32 RoomSize);

  // 조명 생성
  APointLight *SpawnLight(UWorld *World, const FVector &Location,
                          float Intensity, TSubclassOf<APointLight> LightClass);
};
