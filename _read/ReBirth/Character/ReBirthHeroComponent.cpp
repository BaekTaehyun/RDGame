// Copyright Epic Games, Inc. All Rights Reserved.

#include "Character/ReBirthHeroComponent.h"
#include "Components/GameFrameworkComponentDelegates.h"
#include "Components/GameFrameworkComponentManager.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/Character.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerController.h"
#include "Input/ReBirthInputComponent.h"
#include "Input/ReBirthInputConfig.h"
#include "InputMappingContext.h"
#include "ReBirthGameplayTags.h"


#include UE_INLINE_GENERATED_CPP_BY_NAME(ReBirthHeroComponent)

namespace ReBirthHero {
static const float LookYawRate = 300.0f;
static const float LookPitchRate = 165.0f;
}; // namespace ReBirthHero

const FName UReBirthHeroComponent::NAME_BindInputsNow(TEXT("BindInputsNow"));
const FName UReBirthHeroComponent::NAME_ActorFeatureName(TEXT("Hero"));

UReBirthHeroComponent::UReBirthHeroComponent(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  bReadyToBindInputs = false;
}

void UReBirthHeroComponent::OnRegister() {
  Super::OnRegister();

  if (!GetPawn<APawn>()) {
    UE_LOG(LogTemp, Error,
           TEXT("[UReBirthHeroComponent::OnRegister] This component has been "
                "added to a blueprint whose base class is not a Pawn. To use "
                "this component, it MUST be placed on a Pawn Blueprint."));
  } else {
    // Register with the init state system early, this will only work if this is
    // a game world
    RegisterInitStateFeature();
  }
}

bool UReBirthHeroComponent::CanChangeInitState(
    UGameFrameworkComponentManager *Manager, FGameplayTag CurrentState,
    FGameplayTag DesiredState) const {
  check(Manager);

  APawn *Pawn = GetPawn<APawn>();

  if (!CurrentState.IsValid() &&
      DesiredState == ReBirthGameplayTags::InitState_Spawned) {
    // As long as we have a real pawn, let us transition
    if (Pawn) {
      return true;
    }
  } else if (CurrentState == ReBirthGameplayTags::InitState_Spawned &&
             DesiredState == ReBirthGameplayTags::InitState_DataAvailable) {
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
  } else if (CurrentState == ReBirthGameplayTags::InitState_DataAvailable &&
             DesiredState == ReBirthGameplayTags::InitState_DataInitialized) {
    // Ready to initialize data
    return true;
  } else if (CurrentState == ReBirthGameplayTags::InitState_DataInitialized &&
             DesiredState == ReBirthGameplayTags::InitState_GameplayReady) {
    return true;
  }

  return false;
}

void UReBirthHeroComponent::HandleChangeInitState(
    UGameFrameworkComponentManager *Manager, FGameplayTag CurrentState,
    FGameplayTag DesiredState) {
  if (CurrentState == ReBirthGameplayTags::InitState_DataAvailable &&
      DesiredState == ReBirthGameplayTags::InitState_DataInitialized) {
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

void UReBirthHeroComponent::OnActorInitStateChanged(
    const FActorInitStateChangedParams &Params) {
  // For now, we don't depend on other features
  // If you add PawnExtensionComponent later, listen to its state changes here
}

void UReBirthHeroComponent::CheckDefaultInitialization() {
  static const TArray<FGameplayTag> StateChain = {
      ReBirthGameplayTags::InitState_Spawned,
      ReBirthGameplayTags::InitState_DataAvailable,
      ReBirthGameplayTags::InitState_DataInitialized,
      ReBirthGameplayTags::InitState_GameplayReady};

  // This will try to progress from spawned through the data initialization
  // stages until it gets to gameplay ready
  ContinueInitStateChain(StateChain);
}

void UReBirthHeroComponent::BeginPlay() {
  Super::BeginPlay();

  // Notifies that we are done spawning, then try the rest of initialization
  ensure(TryToChangeInitState(ReBirthGameplayTags::InitState_Spawned));
  CheckDefaultInitialization();
}

void UReBirthHeroComponent::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  UnregisterInitStateFeature();

  Super::EndPlay(EndPlayReason);
}

void UReBirthHeroComponent::InitializePlayerInput(
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
  for (const FReBirthInputMappingContextAndPriority &Mapping :
       DefaultInputMappings) {
    if (UInputMappingContext *IMC = Mapping.InputMapping.LoadSynchronous()) {
      FModifyContextOptions Options = {};
      Options.bIgnoreAllPressedKeysUntilRelease = false;
      Subsystem->AddMappingContext(IMC, Mapping.Priority, Options);
    }
  }

  // Bind input actions using the input config
  if (InputConfig) {
    UReBirthInputComponent *ReBirthIC =
        Cast<UReBirthInputComponent>(PlayerInputComponent);
    if (ensureMsgf(
            ReBirthIC,
            TEXT("Unexpected Input Component class! Change the input component "
                 "to UReBirthInputComponent or a subclass of it."))) {
      // Add the key mappings
      ReBirthIC->AddInputMappings(InputConfig, Subsystem);

      // Bind ability actions
      TArray<uint32> BindHandles;
      ReBirthIC->BindAbilityActions(
          InputConfig, this, &ThisClass::Input_AbilityInputTagPressed,
          &ThisClass::Input_AbilityInputTagReleased, /*out*/ BindHandles);

      // Bind native actions
      ReBirthIC->BindNativeAction(
          InputConfig, ReBirthGameplayTags::InputTag_Move,
          ETriggerEvent::Triggered, this, &ThisClass::Input_Move,
          /*bLogIfNotFound=*/false);
      ReBirthIC->BindNativeAction(
          InputConfig, ReBirthGameplayTags::InputTag_Look_Mouse,
          ETriggerEvent::Triggered, this, &ThisClass::Input_LookMouse,
          /*bLogIfNotFound=*/false);
      ReBirthIC->BindNativeAction(
          InputConfig, ReBirthGameplayTags::InputTag_Look_Stick,
          ETriggerEvent::Triggered, this, &ThisClass::Input_LookStick,
          /*bLogIfNotFound=*/false);
      ReBirthIC->BindNativeAction(
          InputConfig, ReBirthGameplayTags::InputTag_Jump,
          ETriggerEvent::Triggered, this, &ThisClass::Input_Jump,
          /*bLogIfNotFound=*/false);
      ReBirthIC->BindNativeAction(
          InputConfig, ReBirthGameplayTags::InputTag_Crouch,
          ETriggerEvent::Triggered, this, &ThisClass::Input_Crouch,
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

bool UReBirthHeroComponent::IsReadyToBindInputs() const {
  return bReadyToBindInputs;
}

void UReBirthHeroComponent::Input_AbilityInputTagPressed(
    FGameplayTag InputTag) {
  // TODO: Integrate with Gameplay Ability System when ready
  UE_LOG(LogTemp, Verbose,
         TEXT("[ReBirthHeroComponent] Ability Input Pressed: %s"),
         *InputTag.ToString());
}

void UReBirthHeroComponent::Input_AbilityInputTagReleased(
    FGameplayTag InputTag) {
  // TODO: Integrate with Gameplay Ability System when ready
  UE_LOG(LogTemp, Verbose,
         TEXT("[ReBirthHeroComponent] Ability Input Released: %s"),
         *InputTag.ToString());
}

void UReBirthHeroComponent::Input_Move(
    const FInputActionValue &InputActionValue) {
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

void UReBirthHeroComponent::Input_LookMouse(
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

void UReBirthHeroComponent::Input_LookStick(
    const FInputActionValue &InputActionValue) {
  APawn *Pawn = GetPawn<APawn>();

  if (!Pawn) {
    return;
  }

  const FVector2D Value = InputActionValue.Get<FVector2D>();

  const UWorld *World = GetWorld();
  check(World);

  if (Value.X != 0.0f) {
    Pawn->AddControllerYawInput(Value.X * ReBirthHero::LookYawRate *
                                World->GetDeltaSeconds());
  }

  if (Value.Y != 0.0f) {
    Pawn->AddControllerPitchInput(Value.Y * ReBirthHero::LookPitchRate *
                                  World->GetDeltaSeconds());
  }
}

void UReBirthHeroComponent::Input_Jump(
    const FInputActionValue &InputActionValue) {
  if (ACharacter *Character = GetPawn<ACharacter>()) {
    Character->Jump();
  }
}

void UReBirthHeroComponent::Input_Crouch(
    const FInputActionValue &InputActionValue) {
  if (ACharacter *Character = GetPawn<ACharacter>()) {
    if (Character->bIsCrouched) {
      Character->UnCrouch();
    } else {
      Character->Crouch();
    }
  }
}
