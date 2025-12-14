// Copyright Epic Games, Inc. All Rights Reserved.

#include "RdPlayerCharacter.h"
#include "../RdGame.h"
#include "../RdGameplayTags.h"
#include "Camera/CameraComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/RdInputComponent.h"
#include "Input/RdInputConfig.h"
#include "RdHeroComponent.h"


ARdPlayerCharacter::ARdPlayerCharacter(
    const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  // Create a camera boom
  CameraBoom = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(
      this, TEXT("CameraBoom"));
  CameraBoom->SetupAttachment(RootComponent);
  CameraBoom->TargetArmLength = 400.0f;
  CameraBoom->bUsePawnControlRotation = true;
  CameraBoom->bEnableCameraLag = true;
  CameraBoom->bEnableCameraRotationLag = true;

  // Create a follow camera
  FollowCamera = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(
      this, TEXT("FollowCamera"));
  FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
  FollowCamera->bUsePawnControlRotation = false;

  // Create Hero Component
  HeroComponent = ObjectInitializer.CreateDefaultSubobject<URdHeroComponent>(
      this, TEXT("HeroComponent"));
}

void ARdPlayerCharacter::BeginPlay() {
  Super::BeginPlay();

  // Initialize Camera
  if (CameraBoom) {
    CameraBoom->TargetArmLength = DefaultCameraDistance;
  }
}

void ARdPlayerCharacter::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  // Note: Basic input bindings (Move, Look, Jump, Crouch) are handled
  // automatically by HeroComponent's InitState system We bind additional Combat
  // actions here.

  if (URdInputComponent *RdIC = Cast<URdInputComponent>(PlayerInputComponent)) {
    if (HeroComponent) {
      if (const URdInputConfig *IC = HeroComponent->GetInputConfig()) {
        // Combo Attack
        RdIC->BindNativeAction(IC, RdGameplayTags::InputTag_Attack_Combo,
                               ETriggerEvent::Started, this,
                               &ThisClass::DoComboAttackStart, false);

        // Charged Attack
        RdIC->BindNativeAction(IC, RdGameplayTags::InputTag_Attack_Charged,
                               ETriggerEvent::Started, this,
                               &ThisClass::DoChargedAttackStart, false);
        RdIC->BindNativeAction(IC, RdGameplayTags::InputTag_Attack_Charged,
                               ETriggerEvent::Completed, this,
                               &ThisClass::DoChargedAttackEnd, false);

        // Camera Toggle
        RdIC->BindNativeAction(IC, RdGameplayTags::InputTag_Camera_Toggle,
                               ETriggerEvent::Triggered, this,
                               &ThisClass::DoCameraToggle, false);
      }
    }
  } else {
    UE_LOG(LogRdGame, Error, TEXT("'%s' Failed to find RdInputComponent!"),
           *GetNameSafe(this));
  }
}

void ARdPlayerCharacter::DoMove(float Right, float Forward) {
  if (Controller != nullptr) {
    const FRotator Rotation = Controller->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);

    const FVector ForwardDirection =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);
    const FVector RightDirection =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    AddMovementInput(ForwardDirection, Forward);
    AddMovementInput(RightDirection, Right);
  }
}

void ARdPlayerCharacter::DoLook(float Yaw, float Pitch) {
  if (Controller != nullptr) {
    AddControllerYawInput(Yaw);
    AddControllerPitchInput(Pitch);
  }
}

void ARdPlayerCharacter::DoJumpStart() { Jump(); }

void ARdPlayerCharacter::DoJumpEnd() { StopJumping(); }

void ARdPlayerCharacter::DoComboAttackStart() {
  // Delegate to Base Logic
  Super::DoComboAttackStart();
}

void ARdPlayerCharacter::DoComboAttackEnd() { Super::DoComboAttackEnd(); }

void ARdPlayerCharacter::DoChargedAttackStart() {
  Super::DoChargedAttackStart();
}

void ARdPlayerCharacter::DoChargedAttackEnd() { Super::DoChargedAttackEnd(); }

void ARdPlayerCharacter::DoCameraToggle() { BP_ToggleCamera(); }

void ARdPlayerCharacter::HandleDeath() {
  Super::HandleDeath();

  // Pull back camera on death
  if (CameraBoom) {
    CameraBoom->TargetArmLength = DeathCameraDistance;
  }
}
