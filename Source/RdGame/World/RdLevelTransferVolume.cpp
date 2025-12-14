
#include "RdLevelTransferVolume.h"
#include "Experience/RdSessionSubsystem.h" // Include our new subsystem
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"


ARdLevelTransferVolume::ARdLevelTransferVolume() {
  PrimaryActorTick.bCanEverTick = false;

  TransferTrigger =
      CreateDefaultSubobject<UBoxComponent>(TEXT("TransferTrigger"));
  RootComponent = TransferTrigger;
  TransferTrigger->SetBoxExtent(FVector(100.f, 100.f, 100.f));
  TransferTrigger->SetCollisionProfileName(TEXT("Trigger"));

  Arrow = CreateDefaultSubobject<UArrowComponent>(TEXT("Arrow"));
  Arrow->SetupAttachment(RootComponent);
}

void ARdLevelTransferVolume::BeginPlay() {
  Super::BeginPlay();

  // Register overlap event
  TransferTrigger->OnComponentBeginOverlap.AddDynamic(
      this, &ARdLevelTransferVolume::OnOverlapBegin);
}

void ARdLevelTransferVolume::OnOverlapBegin(UPrimitiveComponent *OverlappedComp,
                                            AActor *OtherActor,
                                            UPrimitiveComponent *OtherComp,
                                            int32 OtherBodyIndex,
                                            bool bFromSweep,
                                            const FHitResult &SweepResult) {
  // Check if it's a player character
  if (ACharacter *Char = Cast<ACharacter>(OtherActor)) {
    // Check if it's locally controlled (we only want the local client to
    // trigger travel)
    if (Char->IsLocallyControlled()) {
      if (URdSessionSubsystem *SessionSubsystem =
              URdSessionSubsystem::Get(this)) {
        if (DestinationAsset) {
          // Case 1: Direct Asset Reference (Preferred if Tag lookup implies
          // async loading issues) We use the Tag from the asset to be
          // consistent with the system API
          UE_LOG(LogTemp, Log, TEXT("[RdTransfer] Requesting via Asset: %s"),
                 *DestinationAsset->ExperienceId.ToString());
          SessionSubsystem->RequestExperience(DestinationAsset->ExperienceId);
        } else if (DestinationTag.IsValid()) {
          // Case 2: Tag-based lookup
          UE_LOG(LogTemp, Log, TEXT("[RdTransfer] Requesting via Tag: %s"),
                 *DestinationTag.ToString());
          SessionSubsystem->RequestExperience(DestinationTag);
        } else {
          UE_LOG(LogTemp, Warning,
                 TEXT("[RdTransfer] No Destination Tag or Asset configured!"));
        }
      }
    }
  }
}
