
#include "ReceiverStrategy.h"
#include "../Character/RdCharacterMovementComponent.h"
#include "../RdGameCharacter.h"
#include "Components/CapsuleComponent.h"
#include "Engine/World.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/KismetMathLibrary.h"


void FReceiverStrategy::Initialize(ACharacter *InCharacter) {
  OwnerCharacter = InCharacter;
  // Initialization handled by component mode switch
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

  if (auto *RdCMC =
          Cast<URdCharacterMovementComponent>(Char->GetCharacterMovement())) {
    RdCMC->SetNetworkTarget(NewLoc, NewRot, NewVel);
  }
}
