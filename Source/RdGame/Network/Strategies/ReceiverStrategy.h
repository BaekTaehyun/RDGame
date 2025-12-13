#pragma once

#include "CoreMinimal.h"
#include "MovementStrategy.h"

/**
 * Receiver (Simulated Proxy) 전략
 * 역할: 서버로부터 받은 위치/회전 정보를 바탕으로 부드러운 보간(Interpolation)
 * 수행 물리 연산을 수행하지 않음.
 */
class FReceiverStrategy : public IMovementStrategy {
public:
  virtual void Initialize(ACharacter *InCharacter) override;
  virtual void Tick(float DeltaTime) override;
  virtual void OnNetworkDataReceived(const FVector &NewLoc,
                                     const FRotator &NewRot,
                                     const FVector &NewVel,
                                     float Timestamp) override;

private:
  TWeakObjectPtr<ACharacter> OwnerCharacter;

  // 목표 상태
  FVector TargetLocation;
  FRotator TargetRotation;
  FVector TargetVelocity;

  // 보간 설정
  float InterpSpeed = 10.0f;       // 보간 속도
  float RotateInterpSpeed = 15.0f; // 회전 보간 속도 (더 빨라야 자연스러움)
};
