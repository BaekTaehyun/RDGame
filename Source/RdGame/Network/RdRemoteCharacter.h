
#pragma once

#include "CoreMinimal.h"
#include "RdGameCharacter.h"
#include "RdRemoteCharacter.generated.h"

class UGsNetworkMovementComponent;

/**
 * 타 유저(Remote Player) 캐릭터
 *
 * 두 가지 네트워크 모드를 지원합니다:
 * 1. CustomTCP 모드 (필드): GsNetworkMovementComponent를 통해 위치 동기화
 * 2. UnrealUDP 모드 (던전): 언리얼 엔진의 기본 Replication 사용
 */
UENUM(BlueprintType)
enum class ENetworkDriverMode : uint8 {
  None,
  CustomTCP, // 자체 TCP 통신으로 동기화 (Field)
  UnrealUDP  // 언리얼 Replication으로 동기화 (Dungeon)
};

UCLASS()
class RDGAME_API ARdRemoteCharacter : public ARdGameCharacter {
  GENERATED_BODY()

public:
  ARdRemoteCharacter(const FObjectInitializer &ObjectInitializer);

protected:
  virtual void BeginPlay() override;

public:
  // 네트워크 모드 설정
  UFUNCTION(BlueprintCallable, Category = "Network")
  void SetNetworkDriverMode(ENetworkDriverMode NewMode);

  // 현재 네트워크 모드 반환
  UFUNCTION(BlueprintPure, Category = "Network")
  ENetworkDriverMode GetNetworkDriverMode() const { return CurrentDriverMode; }

  // 원격 세션 ID 설정 (스폰 시 호출)
  void SetSessionId(uint32 InSessionId) { RemoteSessionId = InSessionId; }
  uint32 GetSessionId() const { return RemoteSessionId; }

protected:
  // 커스텀 이동 동기화 컴포넌트 (TCP 모드용)
  // 커스텀 이동 동기화 컴포넌트 (TCP 모드용)는 부모 클래스(ARdGameCharacter)의
  // NetworkMovementComponent를 사용합니다.

private:
  // 현재 동작 모드
  ENetworkDriverMode CurrentDriverMode;

  // 원격 유저 세션 ID
  uint32 RemoteSessionId = 0;
};
