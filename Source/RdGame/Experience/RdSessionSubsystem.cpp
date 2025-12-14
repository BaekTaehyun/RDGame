
#include "RdSessionSubsystem.h"
#include "Blueprint/UserWidget.h"
#include "Engine/AssetManager.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "RdExperienceDefinition.h"
#include "RdGameplayTags.h"


// Assuming GsNetworkManager is available to save state
#include "Network/GsNetworkManager.h"

void URdSessionSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
  UE_LOG(LogTemp, Log, TEXT("[RdSession] Initialized."));
}

void URdSessionSubsystem::Deinitialize() { Super::Deinitialize(); }

URdSessionSubsystem *
URdSessionSubsystem::Get(const UObject *WorldContextObject) {
  if (!WorldContextObject || !GEngine)
    return nullptr;
  UWorld *World = GEngine->GetWorldFromContextObject(
      WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
  return World ? World->GetGameInstance()->GetSubsystem<URdSessionSubsystem>()
               : nullptr;
}

void URdSessionSubsystem::RequestExperience(FGameplayTag ExperienceTag) {
  UE_LOG(LogTemp, Log, TEXT("[RdSession] Requesting Experience: %s"),
         *ExperienceTag.ToString());

  // 1. Find Definition
  const URdExperienceDefinition *ExpDef =
      FindExperienceDefinition(ExperienceTag);
  if (!ExpDef) {
    UE_LOG(LogTemp, Error,
           TEXT("[RdSession] Failed to find Experience Definition for tag: %s"),
           *ExperienceTag.ToString());
    return;
  }

  // 2. Broadcast Start (UI should fade in here)
  if (OnTravelRequestStarted.IsBound()) {
    OnTravelRequestStarted.Broadcast(ExpDef);
  }

  // 3. Show Loading Screen (Immediate fallback if no UI listener)
  if (ExpDef->LoadingScreenWidget) {
    if (APlayerController *PC =
            GetGameInstance()->GetFirstLocalPlayerController()) {
      if (UUserWidget *Widget =
              CreateWidget<UUserWidget>(PC, ExpDef->LoadingScreenWidget)) {
        Widget->AddToViewport(9999); // Top Z-Order
      }
    }
  }

  // 4. Save Network State (TCP)
  // IMPORTANT: Persist TCP connection across travel
  if (UGsNetworkManager *NetManager =
          GetGameInstance()->GetSubsystem<UGsNetworkManager>()) {
    // TODO: NetManager->PrepareForTravel(); // If we add this function later
    // For now, GsNetworkManager persists automatically as
    // GameInstanceSubsystem. potentially Clear Remote Actors logic here
  }

  // 5. Execute Travel (Delayed slightly to allow UI to appear? No, Engine
  // blocks anyway) In a real scenario, you might want to wait for the Fade Out
  // animation to finish. For now, we execute immediately.
  ExecuteTravel(ExpDef);
}

void URdSessionSubsystem::ExecuteTravel(const URdExperienceDefinition *ExpDef) {
  if (!ExpDef)
    return;

  FString URL = ExpDef->GetConnectionURL();
  UE_LOG(LogTemp, Log, TEXT("[RdSession] Executing ClientTravel to: %s"), *URL);

  if (APlayerController *PC =
          GetGameInstance()->GetFirstLocalPlayerController()) {
    // Absolute travel creates a new World, connecting to the server.
    PC->ClientTravel(URL, TRAVEL_Absolute);
  }
}

const URdExperienceDefinition *
URdSessionSubsystem::FindExperienceDefinition(FGameplayTag Tag) {
  // Simple verification implementation:
  // In a real project, use UAssetManager::Get().GetPrimaryAssetIdList(...) to
  // find assets. For now, we will assume they are manually registered or we
  // just scan loaded ones.

  // TEMPORARY: Since we don't have the AssetManager setup for this project yet,
  // we assume the GameInstance might hold a list or we just iterate known
  // paths. Or, for this feasibility prototype, we can't easily find unloaded
  // assets without AssetRegistry.

  // Hack: We rely on the Developer to have "loaded" the asset (referenced by
  // something). Better approach: Use AssetRegistry to search for
  // URdExperienceDefinition classes.

  // For this prototype, I will return nullptr if not found.
  // The user will need to configure the TransferVolume with a hard reference in
  // the future, OR we change TransferVolume to hold the
  // `URdExperienceDefinition*` directly instead of Tag. BUT the requirement was
  // Tag-based.

  // Let's iterate all loaded objects of this class.
  for (TObjectIterator<URdExperienceDefinition> It; It; ++It) {
    if (It->ExperienceId == Tag) {
      return *It;
    }
  }

  UE_LOG(
      LogTemp, Warning,
      TEXT(
          "[RdSession] Could not find loaded Experience Asset for tag %s. Make "
          "sure the Asset is referenced by the GameInstance or a Manager."),
      *Tag.ToString());
  return nullptr;
}
