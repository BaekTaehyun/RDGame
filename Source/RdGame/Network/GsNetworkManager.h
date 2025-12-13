#pragma once

#include "CoreMinimal.h"
#include "Network/Protocol.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GsNetworkManager.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnLoginResult, bool, bSuccess);

/**
 * 게임 내 네트워크 로직 처리 (패킷 핸들링, 액터 스폰/동기화)
 */
UCLASS()
class RDGAME_API UGsNetworkManager : public UGameInstanceSubsystem {
  GENERATED_BODY()

public:
  virtual void Initialize(FSubsystemCollectionBase &Collection) override;
  virtual void Deinitialize() override;

  // 블루프린트 접근 헬퍼
  UFUNCTION(BlueprintPure, Category = "Network",
            meta = (WorldContext = "WorldContextObject"))
  static UGsNetworkManager *Get(const UObject *WorldContextObject);

  // 로그인 결과 델리게이트
  UPROPERTY(BlueprintAssignable, Category = "Network")
  FOnLoginResult OnLoginResult;

  // 패킷 핸들러
  void HandleLoginRes(const TArray<uint8> &Data);
  void HandleUserEnter(const TArray<uint8> &Data);
  void HandleUserLeave(const TArray<uint8> &Data);
  void HandleMoveBroadcast(const TArray<uint8> &Data);

private:
  // 원격 플레이어 관리
  UPROPERTY()
  TMap<uint32, AActor *> RemoteActors;

  // 내 세션 ID
  uint32 MySessionId = 0;

  // 스폰할 액터 클래스 (BP 클래스 경로 로드 예정)
  TSubclassOf<AActor> RemoteActorClass;
};
