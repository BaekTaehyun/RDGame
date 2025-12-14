#include "RdRemoteCharacter.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GsNetworkMovementComponent.h"

ARdRemoteCharacter::ARdRemoteCharacter(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
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

#include "../Character/RdCharacterMovementComponent.h"

void ARdRemoteCharacter::SetNetworkDriverMode(ENetworkDriverMode NewMode) {
  CurrentDriverMode = NewMode;

  // 1. Handle Actor-level replication logic
  if (CurrentDriverMode == ENetworkDriverMode::CustomTCP) {
    SetReplicates(false);
    SetReplicateMovement(false);
  } else {
    SetReplicates(true);
    SetReplicateMovement(true);
  }

  // 2. Delegate movement mode logic to the custom component
  if (auto *RdCMC = GetRdCharacterMovement()) {
    RdCMC->SetNetworkDriverMode(CurrentDriverMode);
  }

  // 3. Toggle Network Movement Component (The listener for TCP data)
  UGsNetworkMovementComponent *MoveComp = GetNetworkMovementComponent();
  if (MoveComp) {
    if (CurrentDriverMode == ENetworkDriverMode::CustomTCP) {
      MoveComp->SetComponentTickEnabled(true);
      MoveComp->Activate(true);
    } else {
      MoveComp->SetComponentTickEnabled(false);
      MoveComp->Deactivate();
    }
  }

  UE_LOG(LogTemp, Log, TEXT("RemoteCharacter[%s] Network Mode Changed to: %d"),
         *GetName(), (int32)NewMode);
}
