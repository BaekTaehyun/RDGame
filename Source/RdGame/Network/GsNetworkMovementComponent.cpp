#include "Network/GsNetworkMovementComponent.h"
#include "GameFramework/Character.h"
#include "Strategies/ReceiverStrategy.h"
#include "Strategies/SenderStrategy.h"

UGsNetworkMovementComponent::UGsNetworkMovementComponent() {
  PrimaryComponentTick.bCanEverTick = true;
  PrimaryComponentTick.TickGroup = TG_PostPhysics;
}

void UGsNetworkMovementComponent::BeginPlay() {
  Super::BeginPlay();

  ACharacter *Owner = Cast<ACharacter>(GetOwner());
  if (Owner) {
    // Role에 따라 전략 선택
    // IsLocallyControlled()는 Controller가 있을 때 true.
    // Remote Client(Simulated Proxy)는 Controller가 없음(혹은 AI).
    if (Owner->IsLocallyControlled()) {
      MovementStrategy = MakeShared<FSenderStrategy>();
      UE_LOG(LogTemp, Log,
             TEXT("[GsVisualMovement] Initialized as Sender Strategy"));
    } else {
      MovementStrategy = MakeShared<FReceiverStrategy>();
      UE_LOG(LogTemp, Log,
             TEXT("[GsVisualMovement] Initialized as Receiver Strategy"));
    }
    MovementStrategy->Initialize(Owner);
  }
}

void UGsNetworkMovementComponent::TickComponent(
    float DeltaTime, ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

  if (MovementStrategy.IsValid()) {
    MovementStrategy->Tick(DeltaTime);
  }
}

void UGsNetworkMovementComponent::OnNetworkDataReceived(const FVector &NewLoc,
                                                        const FRotator &NewRot,
                                                        const FVector &NewVel,
                                                        float Timestamp) {
  if (MovementStrategy.IsValid()) {
    MovementStrategy->OnNetworkDataReceived(NewLoc, NewRot, NewVel, Timestamp);
  }
}
