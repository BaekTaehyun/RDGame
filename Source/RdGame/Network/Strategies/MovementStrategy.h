#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"

/**
 * 이동 로직의 공통 인터페이스
 * Sender(내 캐릭터)와 Receiver(타인 캐릭터)가 서로 다른 구현을 가짐
 */
class IMovementStrategy {
public:
  virtual ~IMovementStrategy() {}

  // 초기화 (소유자 캐릭터 설정 등)
  virtual void Initialize(ACharacter *InCharacter) = 0;

  // 매 프레임 이동 로직 처리
  virtual void Tick(float DeltaTime) = 0;

  // 서버로부터 수신한 이동 데이터 처리 (Receiver용, Sender는 Reconciliation 용)
  virtual void OnNetworkDataReceived(const FVector &NewLoc,
                                     const FRotator &NewRot,
                                     const FVector &NewVel,
                                     float Timestamp) = 0;
};
