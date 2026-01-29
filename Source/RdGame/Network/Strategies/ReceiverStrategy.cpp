
#include "ReceiverStrategy.h"
#include "../Character/RdCharacterMovementComponent.h"
#include "../RdGameCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "HAL/PlatformTime.h"
#include "Kismet/KismetMathLibrary.h"


void FReceiverStrategy::Initialize(ACharacter *InCharacter) {
  OwnerCharacter = InCharacter;
  bFirstPacket = true;
  LastReceivedVelocity = FVector::ZeroVector;
}

void FReceiverStrategy::Tick(float DeltaTime) {
  // Logic delegated to URdCharacterMovementComponent::TickComponent
}

void FReceiverStrategy::OnNetworkDataReceived(const FVector &NewLoc,
                                              const FRotator &NewRot,
                                              const FVector &NewVel,
                                              float Timestamp) {
  ACharacter *Char = OwnerCharacter.Get();
  if (!Char)
    return;

  float CurrentLocalTime = FPlatformTime::Seconds();
  float ElapsedSec = 0.0f;

  // ═══════════════════════════════════════════════════════════════════
  // 1. 경과 시간 계산 (BasePredictionTime + Jitter 방식)
  //    - BasePredictionTime: 기본 예측 시간 (RTT/2 ≈ 100ms)
  //    - Jitter: 네트워크 변동에 따른 추가 보정
  // ═══════════════════════════════════════════════════════════════════
  if (bFirstPacket) {
    // 첫 패킷: 기본 예측 시간만 사용
    ElapsedSec = BasePredictionTime;
    bFirstPacket = false;
  } else {
    // 이후 패킷: 패킷 간 시간 차이로 Jitter 계산
    // 서버 타임스탬프 간 차이 = 서버에서 두 패킷 사이의 시간
    float ServerDelta = (Timestamp - LastPacketTimestamp) / 1000.0f;
    // 클라이언트에서 두 패킷 사이의 시간
    float LocalDelta = CurrentLocalTime - LastPacketLocalTime;
    
    // Jitter = 양수면 패킷 지연됨, 음수면 빨리 옴
    float Jitter = LocalDelta - ServerDelta;
    
    // 기본 예측 시간 + Jitter = 실제 예측 시간
    // 안정적인 네트워크에서도 BasePredictionTime만큼은 항상 예측
    ElapsedSec = BasePredictionTime + Jitter;
    
    // 음수 또는 과도한 예측 방지
    ElapsedSec = FMath::Clamp(ElapsedSec, 0.0f, MaxPredictionTime);
  }

  // 상태 저장
  LastPacketLocalTime = CurrentLocalTime;
  LastPacketTimestamp = Timestamp;

  // ═══════════════════════════════════════════════════════════════════
  // 2. 기본 예측 위치 계산 (속도 × 경과시간)
  // ═══════════════════════════════════════════════════════════════════
  FVector PredictedLocation = NewLoc + NewVel * ElapsedSec;

  // ═══════════════════════════════════════════════════════════════════
  // 3. 중력 적용 (점프/낙하 시 포물선 보정)
  // ═══════════════════════════════════════════════════════════════════
  if (FMath::Abs(NewVel.Z) > 10.0f) { // Z축 속도가 있을 때만
    // 포물선 운동: Z = Z0 + Vz*t + 0.5*g*t²
    PredictedLocation.Z = NewLoc.Z + NewVel.Z * ElapsedSec 
                          + 0.5f * GravityZ * ElapsedSec * ElapsedSec;
  }

  // ═══════════════════════════════════════════════════════════════════
  // 4. 방향 전환 보정 (90% 목적지 규칙)
  // ═══════════════════════════════════════════════════════════════════
  if (LastReceivedVelocity.SizeSquared() > 1.0f && NewVel.SizeSquared() > 1.0f) {
    float DotProduct = FVector::DotProduct(
        LastReceivedVelocity.GetSafeNormal(), 
        NewVel.GetSafeNormal());
    
    // 방향이 45도 이상 변했으면 (cos(45°) ≈ 0.7)
    if (DotProduct < DirectionChangeThreshold) {
      FVector CurrentLoc = Char->GetActorLocation();
      FVector ToTarget = PredictedLocation - CurrentLoc;
      float Distance = ToTarget.Size();
      
      if (Distance > 1.0f) {
        // 목적지를 90% 지점으로 단축
        PredictedLocation = CurrentLoc + ToTarget.GetSafeNormal() * Distance * DestinationShortenRatio;
      }
    }
  }

  // ═══════════════════════════════════════════════════════════════════
  // 5. 이상값 검증 (순간이동 방지)
  // ═══════════════════════════════════════════════════════════════════
  FVector CurrentLoc = Char->GetActorLocation();
  float MaxPossibleDistance = NewVel.Size() * (ElapsedSec + 0.5f) + 100.0f; // 여유 100cm
  
  if (FVector::Dist(CurrentLoc, PredictedLocation) > MaxPossibleDistance) {
    // 비정상적으로 먼 거리 → 원본 위치 사용
    PredictedLocation = NewLoc;
  }

  // 속도 저장 (다음 프레임 방향 전환 감지용)
  LastReceivedVelocity = NewVel;

  // ═══════════════════════════════════════════════════════════════════
  // 6. 최종 적용
  // ═══════════════════════════════════════════════════════════════════
  if (auto *RdCMC =
          Cast<URdCharacterMovementComponent>(Char->GetCharacterMovement())) {
    RdCMC->SetNetworkTarget(PredictedLocation, NewRot, NewVel);
  }
}
