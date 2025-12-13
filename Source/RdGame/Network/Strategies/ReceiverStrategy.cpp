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

  // 타겟과의 거리가 너무 멀면(텔레포트 등) 즉시 이동
  float DistSq = FVector::DistSquared(CurrentLoc, TargetLocation);
  if (DistSq > 500.0f * 500.0f) // 5미터 이상 차이나면
  {
    Char->SetActorLocation(TargetLocation);
    Char->SetActorRotation(TargetRotation);
  } else {
    FVector NewLoc =
        FMath::VInterpTo(CurrentLoc, TargetLocation, DeltaTime, InterpSpeed);
    Char->SetActorLocation(NewLoc);
  }

  // 2. 회전 보간 (RInterpTo or Slerp)
  FRotator CurrentRot = Char->GetActorRotation();
  FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRotation, DeltaTime,
                                     RotateInterpSpeed);
  Char->SetActorRotation(NewRot);

  // 3. 애니메이션을 위한 Velocity 설정 (선택)
  // 실제 이동에 따른 속도를 계산하거나, 서버에서 받은 속도를 세팅해줌
  // (보통 CharacterMovementComponent가 위치 변화에 따라 Velocity를 자동
  // 갱신하기도 함)
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
