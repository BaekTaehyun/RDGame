#include "SenderStrategy.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GsNetworkSubsystem.h"
#include "Network/GsNetworkManager.h"
#include "Network/Protocol.h"

void FSenderStrategy::Initialize(ACharacter *InCharacter) {
  OwnerCharacter = InCharacter;
  if (ACharacter *Char = OwnerCharacter.Get()) {
    LastSentLocation = Char->GetActorLocation();
    LastSentRotation = Char->GetActorRotation();
  }
}

void FSenderStrategy::Tick(float DeltaTime) {
  ACharacter *Char = OwnerCharacter.Get();
  if (!Char)
    return;

  // 1. Prediction은 ACharacter 내부의 CharacterMovementComponent가 이미 수행 중

  // 2. 패킷 전송 조건 체크
  TimeSinceLastUpdate += DeltaTime;

  bool bShouldSend = false;

  // 일정 시간 지남
  if (TimeSinceLastUpdate >= UpdateInterval) {
    bShouldSend = true;
  }
  // 또는 위치/회전이 많이 변함 (반응성 향상)
  else {
    float DistSq =
        FVector::DistSquared(Char->GetActorLocation(), LastSentLocation);
    // 회전 변화 감지 (Pitch/Yaw/Roll 모두 체크)
    bool bRotChanged =
        !LastSentRotation.Equals(Char->GetActorRotation(), 1.0f); // 1도 관용

    if (DistSq > 5.0f * 5.0f || bRotChanged) {
      bShouldSend = true;
    }
  }

  if (bShouldSend) {
    SendMovePacket();
    TimeSinceLastUpdate = 0.0f;
  }
}

void FSenderStrategy::OnNetworkDataReceived(const FVector &NewLoc,
                                            const FRotator &NewRot,
                                            const FVector &NewVel,
                                            float Timestamp) {
  // Reconciliation (위치 보정)
  ACharacter *Char = OwnerCharacter.Get();
  if (!Char)
    return;

  float Dist = FVector::Dist(Char->GetActorLocation(), NewLoc);
  if (Dist > 200.0f) // 오차가 2미터 이상이면 강제 동기화 (Snap)
  {
    Char->SetActorLocation(NewLoc);
    // 회전도 맞춰줄 수 있음 (선택)
    // Char->SetActorRotation(NewRot);
    // UE_LOG(LogTemp, Warning, TEXT("[Sender] Position corrected by Server
    // (Error: %.2f)"), Dist);
  }
}

void FSenderStrategy::SendMovePacket() {
  ACharacter *Char = OwnerCharacter.Get();
  if (!Char)
    return;

  // GsNetworkManager에 의존하지 않고 Subsystem을 직접 쓸 수도 있지만, 편의상
  // Manager Get 사용 주의: Manager->Get은 static 함수라 안전.

  const auto &Loc = Char->GetActorLocation();
  const auto &Vel = Char->GetVelocity();
  const auto &Rot = Char->GetActorRotation();

  // 패킷 작성
  Pkt_MoveUpdate Pkt;
  Pkt.size = sizeof(Pkt_MoveUpdate);
  Pkt.type = (uint16)PacketType::C2S_MOVE_UPDATE;
  Pkt.sessionId = 0; // 서버가 채움
  Pkt.x = Loc.X;
  Pkt.y = Loc.Y;
  Pkt.z = Loc.Z;
  Pkt.vx = Vel.X;
  Pkt.vy = Vel.Y;
  Pkt.vz = Vel.Z;

  // 3축 회전 전송
  Pkt.pitch = Rot.Pitch;
  Pkt.yaw = Rot.Yaw;
  Pkt.roll = Rot.Roll;

  // 타임스탬프
  Pkt.timestamp = (uint64)(FPlatformTime::Seconds() * 1000.0);

  // 전송
  TArray<uint8> Buffer;
  Buffer.Append((uint8 *)&Pkt, sizeof(Pkt));

  if (auto *GI = Char->GetGameInstance()) {
    if (auto *Subsystem = GI->GetSubsystem<UGsNetworkSubsystem>()) {
      Subsystem->Send(Buffer);

      // 상태 갱신
      LastSentLocation = Loc;
      LastSentRotation = Rot;
    }
  }
}
