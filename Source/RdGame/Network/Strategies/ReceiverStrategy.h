#pragma once

#include "CoreMinimal.h"
#include "MovementStrategy.h"

/**
 * Receiver (Simulated Proxy) 전략
 * 역할: 서버로부터 받은 위치/회전 정보를 바탕으로 부드러운 보간(Interpolation)
 * 수행 물리 연산을 수행하지 않음.
 * 
 * Dead Reckoning 기능:
 * - 패킷 간 시간 기반 예측 (시간 동기화 문제 해결)
 * - 방향 전환 시 90% 목적지 보정
 * - 중력 적용 (점프/낙하)
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

  // Dead Reckoning 상태
  float LastPacketLocalTime = 0.0f;  // 마지막 패킷 수신 시 로컬 시간
  float LastPacketTimestamp = 0.0f;  // 마지막 패킷의 서버 타임스탬프
  FVector LastReceivedVelocity = FVector::ZeroVector; // 이전 패킷 속도 (방향 전환 감지)
  bool bFirstPacket = true;

  // 설정 상수
  static constexpr float BasePredictionTime = 0.1f;   // 기본 예측 시간 (RTT/2 ≈ 100ms)
  static constexpr float MaxPredictionTime = 0.5f;    // 최대 예측 시간 (500ms)
  static constexpr float DirectionChangeThreshold = 0.7f; // cos(45°) - 방향 전환 임계값
  static constexpr float DestinationShortenRatio = 0.9f;  // 방향 전환 시 목적지 단축 비율
  static constexpr float GravityZ = -980.0f;          // 중력 가속도 (cm/s²)
};
