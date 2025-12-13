#include "Character/RdHeroComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/RdInputComponent.h"
#include "Input/RdInputConfig.h"
#include "InputMappingContext.h"
#include "RdGameplayTags.h"

#include UE_INLINE_GENERATED_CPP_BY_NAME(RdHeroComponent)

namespace RdHero {
static const float LookYawRate = 300.0f;
static const float LookPitchRate = 165.0f;
}; // namespace RdHero

const FName URdHeroComponent::NAME_BindInputsNow(TEXT("BindInputsNow"));
const FName URdHeroComponent::NAME_ActorFeatureName(TEXT("Hero"));

URdHeroComponent::URdHeroComponent(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  bReadyToBindInputs = false;
}

void URdHeroComponent::OnRegister() {
  Super::OnRegister();

  if (!GetPawn<APawn>()) {
    UE_LOG(LogTemp, Error,
           TEXT("[URdHeroComponent::OnRegister] This component has been "
                "added to a blueprint whose base class is not a Pawn. To use "
                "this component, it MUST be placed on a Pawn Blueprint."));
  } else {
    // Register with the init state system early, this will only work if this is
    // a game world
    RegisterInitStateFeature();
  }
}

bool URdHeroComponent::CanChangeInitState(
    UGameFrameworkComponentManager *Manager, FGameplayTag CurrentState,
    FGameplayTag DesiredState) const {
  check(Manager);

  APawn *Pawn = GetPawn<APawn>();

  if (!CurrentState.IsValid() &&
      DesiredState == RdGameplayTags::InitState_Spawned) {
    // As long as we have a real pawn, let us transition
    if (Pawn) {
      return true;
    }
  } else if (CurrentState == RdGameplayTags::InitState_Spawned &&
             DesiredState == RdGameplayTags::InitState_DataAvailable) {
    // If we're authority or autonomous, we need to wait for a controller
    if (Pawn->GetLocalRole() != ROLE_SimulatedProxy) {
      AController *Controller = GetController<AController>();

      if (!Controller) {
        return false;
      }
    }

    const bool bIsLocallyControlled = Pawn->IsLocallyControlled();
    const bool bIsBot = Pawn->IsBotControlled();

    if (bIsLocallyControlled && !bIsBot) {
      APlayerController *PC = GetController<APlayerController>();

      // The input component and local player is required when locally
      // controlled.
      if (!Pawn->InputComponent || !PC || !PC->GetLocalPlayer()) {
        return false;
      }
    }

    return true;
  } else if (CurrentState == RdGameplayTags::InitState_DataAvailable &&
             DesiredState == RdGameplayTags::InitState_DataInitialized) {
    // Ready to initialize data
    return true;
  } else if (CurrentState == RdGameplayTags::InitState_DataInitialized &&
             DesiredState == RdGameplayTags::InitState_GameplayReady) {
    return true;
  }

  return false;
}

void URdHeroComponent::HandleChangeInitState(
    UGameFrameworkComponentManager *Manager, FGameplayTag CurrentState,
    FGameplayTag DesiredState) {
  if (CurrentState == RdGameplayTags::InitState_DataAvailable &&
      DesiredState == RdGameplayTags::InitState_DataInitialized) {
    APawn *Pawn = GetPawn<APawn>();
    if (!ensure(Pawn)) {
      return;
    }

    if (APlayerController *PC = GetController<APlayerController>()) {
      if (Pawn->InputComponent != nullptr) {
        InitializePlayerInput(Pawn->InputComponent);
      }
    }
  }
}

void URdHeroComponent::OnActorInitStateChanged(
    const FActorInitStateChangedParams &Params) {
  // For now, we don't depend on other features
  // If you add PawnExtensionComponent later, listen to its state changes here
}

void URdHeroComponent::CheckDefaultInitialization() {
  static const TArray<FGameplayTag> StateChain = {
      RdGameplayTags::InitState_Spawned,
      RdGameplayTags::InitState_DataAvailable,
      RdGameplayTags::InitState_DataInitialized,
      RdGameplayTags::InitState_GameplayReady};

  // This will try to progress from spawned through the data initialization
  // stages until it gets to gameplay ready
  ContinueInitStateChain(StateChain);
}

void URdHeroComponent::BeginPlay() {
  Super::BeginPlay();

  // Notifies that we are done spawning, then try the rest of initialization
  ensure(TryToChangeInitState(RdGameplayTags::InitState_Spawned));
  CheckDefaultInitialization();
}

void URdHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  UnregisterInitStateFeature();

  Super::EndPlay(EndPlayReason);
}

void URdHeroComponent::InitializePlayerInput(
    UInputComponent *PlayerInputComponent) {
  check(PlayerInputComponent);

  const APawn *Pawn = GetPawn<APawn>();
  if (!Pawn) {
    return;
  }

  const APlayerController *PC = GetController<APlayerController>();
  check(PC);

  const ULocalPlayer *LP = PC->GetLocalPlayer();
  check(LP);

  UEnhancedInputLocalPlayerSubsystem *Subsystem =
      LP->GetSubsystem<UEnhancedInputLocalPlayerSubsystem>();
  check(Subsystem);

  Subsystem->ClearAllMappings();

  // Add default input mapping contexts
  for (const FRdInputMappingContextAndPriority &Mapping :
       DefaultInputMappings) {
    if (UInputMappingContext *IMC = Mapping.InputMapping.LoadSynchronous()) {
      FModifyContextOptions Options = {};
      Options.bIgnoreAllPressedKeysUntilRelease = false;
      Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
    }
  }

  // Bind input actions using the input config
  if (InputConfig) {
    URdInputComponent *RdIC = Cast<URdInputComponent>(PlayerInputComponent);
    if (ensureMsgf(
            RdIC,
            TEXT("Unexpected Input Component class! Change the input component "
                 "to URdInputComponent or a subclass of it."))) {
      // Add the key mappings
      RdIC->AddInputMappings(InputConfig, Subsystem);

      // Bind ability actions
      TArray<uint32> BindHandles;
      RdIC->BindAbilityActions(InputConfig, this,
                               &ThisClass::Input_AbilityInputTagPressed,
                               &ThisClass::Input_AbilityInputTagReleased,
                               /*out*/ BindHandles);

      // Bind native actions
      RdIC->BindNativeAction(InputConfig, RdGameplayTags::InputTag_Move,
                             ETriggerEvent::Triggered, this,
                             &ThisClass::Input_Move,
                             /*bLogIfNotFound=*/false);
      RdIC->BindNativeAction(InputConfig, RdGameplayTags::InputTag_Look_Mouse,
                             ETriggerEvent::Triggered, this,
                             &ThisClass::Input_LookMouse,
                             /*bLogIfNotFound=*/false);
      RdIC->BindNativeAction(InputConfig, RdGameplayTags::InputTag_Look_Stick,
                             ETriggerEvent::Triggered, this,
                             &ThisClass::Input_LookStick,
                             /*bLogIfNotFound=*/false);
      RdIC->BindNativeAction(InputConfig, RdGameplayTags::InputTag_Jump,
                             ETriggerEvent::Triggered, this,
                             &ThisClass::Input_Jump,
                             /*bLogIfNotFound=*/false);
      RdIC->BindNativeAction(InputConfig, RdGameplayTags::InputTag_Crouch,
                             ETriggerEvent::Triggered, this,
                             &ThisClass::Input_Crouch,
                             /*bLogIfNotFound=*/false);
    }
  }

  if (ensure(!bReadyToBindInputs)) {
    bReadyToBindInputs = true;
  }

  UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
      const_cast<APlayerController *>(PC), NAME_BindInputsNow);
  UGameFrameworkComponentManager::SendGameFrameworkComponentExtensionEvent(
      const_cast<APawn *>(Pawn), NAME_BindInputsNow);
}

bool URdHeroComponent::IsReadyToBindInputs() const {
  return bReadyToBindInputs;
}

void URdHeroComponent::Input_AbilityInputTagPressed(FGameplayTag InputTag) {
  // TODO: Integrate with Gameplay Ability System when ready
  UE_LOG(LogTemp, Verbose, TEXT("[RdHeroComponent] Ability Input Pressed: %s"),
         *InputTag.ToString());
}

void URdHeroComponent::Input_AbilityInputTagReleased(FGameplayTag InputTag) {
  // TODO: Integrate with Gameplay Ability System when ready
  UE_LOG(LogTemp, Verbose, TEXT("[RdHeroComponent] Ability Input Released: %s"),
         *InputTag.ToString());
}

void URdHeroComponent::Input_Move(const FInputActionValue &InputActionValue) {
  APawn *Pawn = GetPawn<APawn>();
  AController *Controller = Pawn ? Pawn->GetController() : nullptr;

  if (Controller) {
    const FVector2D Value = InputActionValue.Get<FVector2D>();
    const FRotator MovementRotation(0.0f, Controller->GetControlRotation().Yaw,
                                    0.0f);

    if (Value.X != 0.0f) {
      const FVector MovementDirection =
          MovementRotation.RotateVector(FVector::RightVector);
      Pawn->AddMovementInput(MovementDirection, Value.X);
    }

    if (Value.Y != 0.0f) {
      const FVector MovementDirection =
          MovementRotation.RotateVector(FVector::ForwardVector);
      Pawn->AddMovementInput(MovementDirection, Value.Y);
    }
  }
}

void URdHeroComponent::Input_LookMouse(
    const FInputActionValue &InputActionValue) {
  APawn *Pawn = GetPawn<APawn>();

  if (!Pawn) {
    return;
  }

  const FVector2D Value = InputActionValue.Get<FVector2D>();

  if (Value.X != 0.0f) {
    Pawn->AddControllerYawInput(Value.X);
  }

  if (Value.Y != 0.0f) {
    Pawn->AddControllerPitchInput(Value.Y);
  }
}

void URdHeroComponent::Input_LookStick(
    const FInputActionValue &InputActionValue) {
  APawn *Pawn = GetPawn<APawn>();

  if (!Pawn) {
    return;
  }

  const FVector2D Value = InputActionValue.Get<FVector2D>();

  const UWorld *World = GetWorld();
  check(World);

  if (Value.X != 0.0f) {
    Pawn->AddControllerYawInput(Value.X * RdHero::LookYawRate *
                                World->GetDeltaSeconds());
  }

  if (Value.Y != 0.0f) {
    Pawn->AddControllerPitchInput(Value.Y * RdHero::LookPitchRate *
                                  World->GetDeltaSeconds());
  }
}

void URdHeroComponent::Input_Jump(const FInputActionValue &InputActionValue) {
  if (ACharacter *Character = GetPawn<ACharacter>()) {
    Character->Jump();
  }
}

void URdHeroComponent::Input_Crouch(const FInputActionValue &InputActionValue) {
  if (ACharacter *Character = GetPawn<ACharacter>()) {
    if (Character->bIsCrouched) {
      Character->UnCrouch();
    } else {
      Character->Crouch();
    }
  }
}
