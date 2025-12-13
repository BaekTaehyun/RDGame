#include "RdRemoteCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GsNetworkMovementComponent.h"

ARdRemoteCharacter::ARdRemoteCharacter(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer.DoNotCreateDefaultSubobject(TEXT("CameraBoom"))
                .DoNotCreateDefaultSubobject(TEXT("FollowCamera"))
                .DoNotCreateDefaultSubobject(TEXT("HeroComponent"))) {
  // 기본적으로 AI 컨트롤러를 사용하지 않음 (직접 보간 or Replication)
  AutoPossessAI = EAutoPossessAI::Disabled;
  AIControllerClass = nullptr;

  // 충돌 설정: 폰과는 충돌하되, 카메라는 통과하도록 설정 (필요시 조정)
  GetCapsuleComponent()->SetCollisionProfileName(TEXT("Pawn"));

  // TCP 동기화 컴포넌트 생성 (부모 클래스에서 생성된 컴포넌트 사용)
  // NetworkMovementComponent =
  // CreateDefaultSubobject<UGsNetworkMovementComponent>(TEXT("NetworkMovementComponent"));

  // 기본 모드는 CustomTCP로 설정 (필드 기본값)
  CurrentDriverMode = ENetworkDriverMode::CustomTCP;

  // 메시 설정 (로컬 플레이어와 동일한 에셋 사용 가능)
  // 최적화를 위해 불필요한 컴포넌트(카메라 등)는 상속받았으나 비활성화 고려
  // 가능
}

void ARdRemoteCharacter::BeginPlay() {
  Super::BeginPlay();

  // 초기 모드 적용
  SetNetworkDriverMode(CurrentDriverMode);
}

void ARdRemoteCharacter::SetNetworkDriverMode(ENetworkDriverMode NewMode) {
  CurrentDriverMode = NewMode;

  UGsNetworkMovementComponent *MoveComp = GetNetworkMovementComponent();

  switch (CurrentDriverMode) {
  case ENetworkDriverMode::CustomTCP:
    // 1. 언리얼 리플리케이션 비활성화
    SetReplicates(false);
    SetReplicateMovement(false);

    // 2. 물리/이동 모드 설정
    // CustomTCP 모드에서는 직접 Transform을 보간하므로 물리 시뮬레이션 간섭을
    // 최소화 하지만 애니메이션을 위해 Walking 모드는 유지
    if (auto *CMC = GetCharacterMovement()) {
      // 마찰력과 제동력을 0으로 설정하여 Velocity가 물리 엔진에 의해 초기화되는
      // 것을 방지
      CMC->GroundFriction = 0.0f;
      CMC->BrakingDecelerationWalking = 0.0f;
    }

    // 3. 커스텀 컴포넌트 활성화
    if (MoveComp) {
      MoveComp->SetComponentTickEnabled(true);
      MoveComp->Activate(true);
    }
    break;

  case ENetworkDriverMode::UnrealUDP:
    // 0. 물리 설정 복구 (기본값)
    if (auto *CMC = GetCharacterMovement()) {
      CMC->GroundFriction = 8.0f;                // 기본값 복구
      CMC->BrakingDecelerationWalking = 2048.0f; // 기본값 복구
    }

    // 1. 커스텀 컴포넌트 비활성화
    if (MoveComp) {
      MoveComp->SetComponentTickEnabled(false);
      MoveComp->Deactivate();
    }

    // 2. 언리얼 리플리케이션 활성화
    SetReplicates(true);
    SetReplicateMovement(true);
    break;

  default:
    break;
  }

  UE_LOG(LogTemp, Log, TEXT("RemoteCharacter[%s] Network Mode Changed to: %d"),
         *GetName(), (int32)NewMode);
}
