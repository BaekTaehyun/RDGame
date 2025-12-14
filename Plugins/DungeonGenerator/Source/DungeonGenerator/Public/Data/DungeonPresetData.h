#pragma once

#include "CoreMinimal.h"
#include "CoreTypes.h" // For FIntPoint if needed, or stick to FIntPoint from CoreMinimal
#include "Engine/DataAsset.h"
#include "DungeonPresetData.generated.h"


/**
 * 소켓 연결 방향 정의
 */
UENUM(BlueprintType)
enum class EModuleSocketDirection : uint8 {
  North UMETA(DisplayName = "North (+Y)"),
  East UMETA(DisplayName = "East (+X)"),
  South UMETA(DisplayName = "South (-Y)"),
  West UMETA(DisplayName = "West (-X)")
};

/**
 * 모듈(방)의 유형 정의
 * 알고리즘이 "다음엔 무엇을 붙일까?" 결정할 때 사용됨
 */
UENUM(BlueprintType)
enum class EDungeonPresetModuleType : uint8 {
  Room UMETA(DisplayName = "Room"),
  Corridor UMETA(DisplayName = "Corridor"),
  Hub UMETA(DisplayName = "Hub/Intersection"),
  Start UMETA(DisplayName = "Start Point"),
  Boss UMETA(DisplayName = "Boss Room"),
  DeadEnd UMETA(DisplayName = "Dead End (Cap)")
};

/**
 * 개별 소켓 정보
 * 모듈의 어느 위치에, 어떤 방향으로, 어떤 속성의 문이 있는가?
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FModuleSocket {
  GENERATED_BODY()

public:
  // 모듈 내 로컬 그리드 좌표 (0,0 기준)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
  FIntPoint LocalPosition = FIntPoint(0, 0);

  // 연결 방향 (이 소켓이 바라보는 방향)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
  EModuleSocketDirection Direction = EModuleSocketDirection::North;

  // 소켓 태그 (예: "BigDoor", "Secret". 매칭되는 태그끼리만 연결됨. 비어있으면
  // "Default")
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Socket")
  FName SocketTag = NAME_None;
};

/**
 * 하나의 모듈(방/복도) 데이터 정의
 */
USTRUCT(BlueprintType)
struct DUNGEONGENERATOR_API FModuleData {
  GENERATED_BODY()

public:
  // 모듈 식별자 (디버깅용)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
  FName ModuleID;

  // 모듈 역할 (알고리즘 로직용)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
  EDungeonPresetModuleType Type = EDungeonPresetModuleType::Room;

  // 모듈 크기 (타일 단위)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
  FIntPoint Size = FIntPoint(1, 1);

  // 소켓 리스트
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module")
  TArray<FModuleSocket> Sockets;

  // 실제 스폰될 액터 클래스 (LevelInstance 또는 일반 AActor)
  // 메모리 절약을 위해 Soft Class Reference 사용
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
  TSoftClassPtr<AActor> ModuleActorClass;

  // 선택 가중치 (높을수록 더 자주 선택됨)
  UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Module",
            meta = (ClampMin = "0.0"))
  float SelectionWeight = 1.0f;
};

/**
 * 프리셋 모듈 데이터베이스 에셋
 */
UCLASS(BlueprintType)
class DUNGEONGENERATOR_API UPresetModuleDatabase : public UDataAsset {
  GENERATED_BODY()

public:
  // 등록된 모든 모듈 리스트
  UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dungeon Modules")
  TArray<FModuleData> Modules;

  // 헬퍼 함수: 특정 타입의 모듈만 가져오기
  UFUNCTION(BlueprintCallable, Category = "Dungeon Modules")
  TArray<FModuleData> GetModulesByType(EDungeonPresetModuleType InType) const;
};
