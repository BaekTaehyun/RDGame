#pragma once

#include "CoreMinimal.h"
#include "MovementStrategy.h"

/**
 * Sender (Autonomous Proxy) 전략
 * 역할: 입력 처리, 클라이언트 측 예측(Prediction), 서버로 이동 패킷 전송
 */
class FSenderStrategy : public IMovementStrategy {
public:
  virtual void Initialize(ACharacter *InCharacter) override;
  virtual void Tick(float DeltaTime) override;
  virtual void OnNetworkDataReceived(const FVector &NewLoc,
                                     const FRotator &NewRot,
                                     const FVector &NewVel,
                                     float Timestamp) override;

private:
  // 패킷 전송 주기 관리
  float TimeSinceLastUpdate = 0.0f;
  const float UpdateInterval = 0.1f; // 10Hz

  // 마지막으로 전송한 상태 (중복 전송 방지)
  FVector LastSentLocation;
  FRotator LastSentRotation; // 3축 회전 저장

  // 소유자 캐릭터
  TWeakObjectPtr<ACharacter> OwnerCharacter;

  // 패킷 전송 헬퍼
  void SendMovePacket();
};
