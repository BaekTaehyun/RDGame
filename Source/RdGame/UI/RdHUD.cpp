#include "RdHUD.h"
#include "Components/GameFrameworkComponentManager.h"
#include "NativeGameplayTags.h"

// Define logs if not already defined in project
DEFINE_LOG_CATEGORY_STATIC(LogRdHUD, Log, All);

// Tags for init states
UE_DEFINE_GAMEPLAY_TAG_STATIC(TAG_RdHUD_InitState_UIReady,
                              "HUD.InitState.UIReady");

const FName ARdHUD::HUD_InitState_UIReady = FName("UIReady");

ARdHUD::ARdHUD(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  PrimaryActorTick.bStartWithTickEnabled = false;
}

void ARdHUD::PreInitializeComponents() {
  Super::PreInitializeComponents();

  // Register this actor as a feature receiver so GameFeatureActions can bind to
  // it
  UGameFrameworkComponentManager::AddGameFrameworkComponentReceiver(this);
}

void ARdHUD::BeginPlay() {
  UGameFrameworkComponentManager *Manager =
      GetGameInstance()->GetSubsystem<UGameFrameworkComponentManager>();
  // The RegisterFeatureAgent call is not necessary/valid here as we registered
  // as a receiver in PreInitializeComponents

  Super::BeginPlay();

  // If we needed to defer UI creation, we would check here, but for now we
  // proceed. We might want to use the InitState pattern more strictly if we
  // have complex dependencies. For now, we will just signal that we are ready
  // for UI extensions.

  // Creating UI is often handled by the GameFeatureAction_AddWidgets or passing
  // init states Here we just ensure we reach the state where other things can
  // attach.
  CheckDefaultInitialization();
}

void ARdHUD::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  UGameFrameworkComponentManager::RemoveGameFrameworkComponentReceiver(this);
  Super::EndPlay(EndPlayReason);
}

FName ARdHUD::GetFeatureName() const {
  return ARdHUD::StaticClass()->GetFName();
}

bool ARdHUD::CanChangeInitState(UGameFrameworkComponentManager *Manager,
                                FGameplayTag CurrentState,
                                FGameplayTag DesiredState) const {
  // Simple state machine: always allow transition for now
  return true;
}

void ARdHUD::HandleChangeInitState(UGameFrameworkComponentManager *Manager,
                                   FGameplayTag CurrentState,
                                   FGameplayTag DesiredState) {
  // Logic for state transitions (e.g. creating widgets when reaching a specific
  // state)
}

void ARdHUD::OnActorInitStateChanged(
    const FActorInitStateChangedParams &Params) {
  // React to other actors changing state if needed
}

void ARdHUD::CheckDefaultInitialization() {
  // Simply advance to the ready state
  // In a full implementation, we might wait for the PlayerController or Pawn to
  // be ready

  static const FGameplayTag InitState_GameplayReady =
      FGameplayTag::RequestGameplayTag("InitState.GameplayReady");
  static const FGameplayTag InitState_DataInitialized =
      FGameplayTag::RequestGameplayTag("InitState.DataInitialized");

  UGameFrameworkComponentManager *Manager =
      GetGameInstance()->GetSubsystem<UGameFrameworkComponentManager>();
  if (Manager) {
    // Force move to UI Ready for this phase of potential "waiting"
    // In Lyra, this is much more complex, orchestrating
    // Pawn/Controller/HUD/PlayerState simpler approach: just declare we are
    // ready for extensions.

    // Note using a custom tagline here since we haven't defined the full
    // InitState chain in Tags yet. Using the tag defined above.
    APawn *Pawn = GetOwningPlayerController()
                      ? GetOwningPlayerController()->GetPawn()
                      : nullptr;

    // Just inform the manager we are "available" for extensions.
    // The standard way is SendExtensionEvent.

    // Trigger the "GameActorReady" event which AddWidgets action listens for by
    // default in Lyra (though Lyra uses specific InitStates, simple extension
    // actions just use "GameActorReady")
    Manager->SendExtensionEvent(
        this, UGameFrameworkComponentManager::NAME_GameActorReady);
  }
}

void ARdHUD::CreateUI() {
  // This method might be called by the InitState handler
}
