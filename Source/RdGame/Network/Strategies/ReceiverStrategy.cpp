#include "ReceiverStrategy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"

void FReceiverStrategy::Initialize(ACharacter *InCharacter) {
  OwnerCharacter = InCharacter;
  if (ACharacter *Char = OwnerCharacter.Get()) {
    TargetLocation = Char->GetActorLocation();
    TargetRotation = Char->GetActorRotation();

    // Receiver는 물리를 끄거나 최소화해야 함
    if (auto *MoveComp = Char->GetCharacterMovement()) {
      // 중력 등 물리 연산 방해 금지 (선택 사항)
      // MoveComp->GravityScale = 0.0f;
      // MoveComp->SetMovementMode(MOVE_Custom);
    }
  }
}

void FReceiverStrategy::Tick(float DeltaTime) {
  ACharacter *Char = OwnerCharacter.Get();
  if (!Char)
    return;

  // 1. 위치 보간 (VInterpTo)
  FVector CurrentLoc = Char->GetActorLocation();
  FVector NewLoc = CurrentLoc;

  // 타겟과의 거리가 너무 멀면(텔레포트 등) 즉시 이동
  float DistSq = FVector::DistSquared(CurrentLoc, TargetLocation);
  if (DistSq > 500.0f * 500.0f) // 5미터 이상 차이나면
  {
    NewLoc = TargetLocation;
    Char->SetActorLocation(TargetLocation);
    Char->SetActorRotation(TargetRotation);
  } else {
    NewLoc =
        FMath::VInterpTo(CurrentLoc, TargetLocation, DeltaTime, InterpSpeed);
    Char->SetActorLocation(NewLoc);
  }

  // 2. 회전 보간 (RInterpTo or Slerp)
  FRotator CurrentRot = Char->GetActorRotation();
  FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRotation, DeltaTime,
                                     RotateInterpSpeed);
  Char->SetActorRotation(NewRot);

  // 3. 애니메이션을 위한 Velocity 설정
  if (auto *MoveComp = Char->GetCharacterMovement()) {
    // A. 실제 이동 속도 기반 (Moonwalk 방지)
    FVector RealVelocity = (NewLoc - CurrentLoc) / DeltaTime;

    // B. 서버 속도와 혼합하거나(Blending), 특정 상태에서는 강제 설정
    // 여기서는 실제 이동 속도 반영이 애니메이션에 가장 자연스러움
    // Z축은 점프 등을 위해 서버 속도 참고 가능하나, 단순화를 위해 RealVelocity
    // 사용
    MoveComp->Velocity = RealVelocity;

    // UpdateComponent를 호출하여 파생 데이터 갱신
    MoveComp->UpdateComponentVelocity();
  }
}

void FReceiverStrategy::OnNetworkDataReceived(const FVector &NewLoc,
                                              const FRotator &NewRot,
                                              const FVector &NewVel,
                                              float Timestamp) {
  TargetLocation = NewLoc;
  TargetRotation = NewRot;
  TargetVelocity = NewVel;

  // Dead Reckoning (추측 항법) - 선택사항
  // 핑(Ping) 고려: Timestamp와 현재 시간 차이를 이용해 미래 위치 예측
  // TargetLocation += TargetVelocity * (Ping / 1000.0f);
}
