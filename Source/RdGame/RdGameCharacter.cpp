// Copyright Epic Games, Inc. All Rights Reserved.

#include "RdGameCharacter.h"
#include "Camera/CameraComponent.h"
#include "Character/RdHeroComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/WidgetComponent.h"
#include "Engine/DamageEvents.h"
#include "Engine/LocalPlayer.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/Controller.h"
#include "GameFramework/SpringArmComponent.h"
#include "Input/RdInputComponent.h"
#include "Input/RdInputConfig.h"
#include "InputActionValue.h"
#include "Network/GsNetworkMovementComponent.h"
#include "RdGame.h"
#include "RdGameplayTags.h"
#include "TimerManager.h"
#include "Variant_Combat/UI/CombatLifeBar.h"

ARdGameCharacter::ARdGameCharacter()
    : ARdGameCharacter(FObjectInitializer::Get()) {}

ARdGameCharacter::ARdGameCharacter(const FObjectInitializer &ObjectInitializer)
    : Super(ObjectInitializer) {
  // Set size for collision capsule
  GetCapsuleComponent()->InitCapsuleSize(42.f, 96.0f);

  // Don't rotate when the controller rotates. Let that just affect the camera.
  bUseControllerRotationPitch = false;
  bUseControllerRotationYaw = false;
  bUseControllerRotationRoll = false;

  // Configure character movement
  GetCharacterMovement()->bOrientRotationToMovement =
      true; // Character moves in the direction of input...
  GetCharacterMovement()->RotationRate =
      FRotator(0.0f, 500.0f, 0.0f); // ...at this rotation rate

  // Note: For faster iteration times these variables, and many more, can be
  // tweaked in the Character Blueprint instead of recompiling to adjust them
  GetCharacterMovement()->JumpZVelocity = 500.f;
  GetCharacterMovement()->AirControl = 0.35f;
  GetCharacterMovement()->MaxWalkSpeed = 500.f;
  GetCharacterMovement()->MinAnalogWalkSpeed = 20.f;
  GetCharacterMovement()->BrakingDecelerationWalking = 2000.f;
  GetCharacterMovement()->BrakingDecelerationFalling = 1500.0f;

  // Create a camera boom (pulls in towards the player if there is a collision)
  CameraBoom = ObjectInitializer.CreateDefaultSubobject<USpringArmComponent>(
      this, TEXT("CameraBoom"));
  CameraBoom->SetupAttachment(RootComponent);
  CameraBoom->TargetArmLength =
      400.0f; // The camera follows at this distance behind the character
  CameraBoom->bUsePawnControlRotation =
      true; // Rotate the arm based on the controller

  // Create a follow camera
  FollowCamera = ObjectInitializer.CreateDefaultSubobject<UCameraComponent>(
      this, TEXT("FollowCamera"));
  FollowCamera->SetupAttachment(
      CameraBoom,
      USpringArmComponent::SocketName); // Attach the camera to the end of the
                                        // boom and let the boom adjust to match
                                        // the controller orientation
  FollowCamera->bUsePawnControlRotation =
      false; // Camera does not rotate relative to arm

  // Create Network Movement Component
  NetworkMovementComponent =
      ObjectInitializer.CreateDefaultSubobject<UGsNetworkMovementComponent>(
          this, TEXT("NetworkMovementComponent"));

  // Create Hero Component
  HeroComponent = ObjectInitializer.CreateDefaultSubobject<URdHeroComponent>(
      this, TEXT("HeroComponent"));

  // create the life bar widget component
  LifeBar = ObjectInitializer.CreateDefaultSubobject<UWidgetComponent>(
      this, TEXT("LifeBar"));
  LifeBar->SetupAttachment(RootComponent);

  // set the player tag
  Tags.Add(FName("Player"));

  // bind the attack montage ended delegate
  OnAttackMontageEnded.BindUObject(this, &ARdGameCharacter::AttackMontageEnded);

  // Configure camera lag
  CameraBoom->bEnableCameraLag = true;
  CameraBoom->bEnableCameraRotationLag = true;

  // Note: The skeletal mesh and anim blueprint references on the Mesh component
  // (inherited from Character) are set in the derived blueprint asset named
  // ThirdPersonCharacter (to avoid direct content references in C++)
}

void ARdGameCharacter::SetupPlayerInputComponent(
    UInputComponent *PlayerInputComponent) {
  // NOTE: 기본 입력 바인딩(Move, Look, Jump, Crouch)은 HeroComponent의
  // InitState 시스템이 HandleChangeInitState에서 자동으로 처리합니다.
  // SetupPlayerInputComponent는 ACharacter::SetupPlayerInputComponent 이후에
  // 호출되므로, 추가 Combat 관련 바인딩만 여기서 처리합니다.

  // Bind Native Combat Actions (공격, 카메라 토글 등 캐릭터 고유 액션)
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
    UE_LOG(LogRdGame, Error,
           TEXT("'%s' Failed to find RdInputComponent! This character is built "
                "to use URdInputComponent. Check the Player Controller's "
                "InputComponentClass setting."),
           *GetNameSafe(this));
  }
}

void ARdGameCharacter::DoMove(float Right, float Forward) {
  if (GetController() != nullptr) {
    // find out which way is forward
    const FRotator Rotation = GetController()->GetControlRotation();
    const FRotator YawRotation(0, Rotation.Yaw, 0);

    // get forward vector
    const FVector ForwardDirection =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::X);

    // get right vector
    const FVector RightDirection =
        FRotationMatrix(YawRotation).GetUnitAxis(EAxis::Y);

    // add movement
    AddMovementInput(ForwardDirection, Forward);
    AddMovementInput(RightDirection, Right);
  }
}

void ARdGameCharacter::DoLook(float Yaw, float Pitch) {
  if (GetController() != nullptr) {
    // add yaw and pitch input to controller
    AddControllerYawInput(Yaw);
    AddControllerPitchInput(Pitch);
  }
}

void ARdGameCharacter::DoJumpStart() {
  // signal the character to jump
  Jump();
}

void ARdGameCharacter::DoJumpEnd() {
  // signal the character to stop jumping
  StopJumping();
}

void ARdGameCharacter::BeginPlay() {
  Super::BeginPlay();

  // get the life bar from the widget component
  if (LifeBar) {
    LifeBarWidget = Cast<UCombatLifeBar>(LifeBar->GetUserWidgetObject());
    // check(LifeBarWidget); // Widget might not be initialized yet in some
    // cases
  }

  // initialize the camera
  GetCameraBoom()->TargetArmLength = DefaultCameraDistance;

  // save the relative transform for the mesh so we can reset the ragdoll later
  MeshStartingTransform = GetMesh()->GetRelativeTransform();

  // set the life bar color
  if (LifeBarWidget) {
    LifeBarWidget->SetBarColor(LifeBarColor);
  }

  // reset HP to maximum
  ResetHP();
}

void ARdGameCharacter::EndPlay(const EEndPlayReason::Type EndPlayReason) {
  Super::EndPlay(EndPlayReason);

  // clear the respawn timer
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().ClearTimer(RespawnTimer);
  }
}

void ARdGameCharacter::NotifyControllerChanged() {
  Super::NotifyControllerChanged();
  // CombatPlayerController logic if needed
}

void ARdGameCharacter::DoComboAttackStart() {
  // are we already playing an attack animation?
  if (bIsAttacking) {
    // cache the input time so we can check it later
    if (UWorld *World = GetWorld()) {
      CachedAttackInputTime = World->GetTimeSeconds();
    }
    return;
  }

  // perform a combo attack
  ComboAttack();
}

void ARdGameCharacter::DoComboAttackEnd() {
  // stub
}

void ARdGameCharacter::DoChargedAttackStart() {
  // raise the charging attack flag
  bIsChargingAttack = true;

  if (bIsAttacking) {
    // cache the input time so we can check it later
    if (UWorld *World = GetWorld()) {
      CachedAttackInputTime = World->GetTimeSeconds();
    }
    return;
  }

  ChargedAttack();
}

void ARdGameCharacter::DoChargedAttackEnd() {
  // lower the charging attack flag
  bIsChargingAttack = false;

  // if we've done the charge loop at least once, release the charged attack
  // right away
  if (bHasLoopedChargedAttack) {
    CheckChargedAttack();
  }
}

void ARdGameCharacter::DoCameraToggle() {
  // call the BP hook
  BP_ToggleCamera();
}

void ARdGameCharacter::ResetHP() {
  // reset the current HP total
  CurrentHP = MaxHP;

  // update the life bar
  if (LifeBarWidget) {
    LifeBarWidget->SetLifePercentage(1.0f);
  }
}

void ARdGameCharacter::ComboAttack() {
  // raise the attacking flag
  bIsAttacking = true;

  // reset the combo count
  ComboCount = 0;

  // notify enemies they are about to be attacked
  NotifyEnemiesOfIncomingAttack();

  // play the attack montage
  if (UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance()) {
    const float MontageLength = AnimInstance->Montage_Play(
        ComboAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f,
        true);

    // subscribe to montage completed and interrupted events
    if (MontageLength > 0.0f) {
      // set the end delegate for the montage
      AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded,
                                           ComboAttackMontage);
    }
  }
}

void ARdGameCharacter::ChargedAttack() {
  // raise the attacking flag
  bIsAttacking = true;

  // reset the charge loop flag
  bHasLoopedChargedAttack = false;

  // notify enemies they are about to be attacked
  NotifyEnemiesOfIncomingAttack();

  // play the charged attack montage
  if (UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance()) {
    const float MontageLength = AnimInstance->Montage_Play(
        ChargedAttackMontage, 1.0f, EMontagePlayReturnType::MontageLength, 0.0f,
        true);

    // subscribe to montage completed and interrupted events
    if (MontageLength > 0.0f) {
      // set the end delegate for the montage
      AnimInstance->Montage_SetEndDelegate(OnAttackMontageEnded,
                                           ChargedAttackMontage);
    }
  }
}

void ARdGameCharacter::AttackMontageEnded(UAnimMontage *Montage,
                                          bool bInterrupted) {
  // reset the attacking flag
  bIsAttacking = false;

  UWorld *World = GetWorld();
  if (!World)
    return;

  // check if we have a non-stale cached input
  if (World->GetTimeSeconds() - CachedAttackInputTime <=
      AttackInputCacheTimeTolerance) {
    // are we holding the charged attack button?
    if (bIsChargingAttack) {
      // do a charged attack
      ChargedAttack();
    } else {
      // do a regular attack
      ComboAttack();
    }
  }
}

void ARdGameCharacter::DoAttackTrace(FName DamageSourceBone) {
  // sweep for objects in front of the character to be hit by the attack
  TArray<FHitResult> OutHits;

  // start at the provided socket location, sweep forward
  const FVector TraceStart = GetMesh()->GetSocketLocation(DamageSourceBone);
  const FVector TraceEnd =
      TraceStart + (GetActorForwardVector() * MeleeTraceDistance);

  // check for pawn and world dynamic collision object types
  FCollisionObjectQueryParams ObjectParams;
  ObjectParams.AddObjectTypesToQuery(ECC_Pawn);
  ObjectParams.AddObjectTypesToQuery(ECC_WorldDynamic);

  // use a sphere shape for the sweep
  FCollisionShape CollisionShape;
  CollisionShape.SetSphere(MeleeTraceRadius);

  // ignore self
  FCollisionQueryParams QueryParams;
  QueryParams.AddIgnoredActor(this);

  if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd,
                                         FQuat::Identity, ObjectParams,
                                         CollisionShape, QueryParams)) {
    // iterate over each object hit
    for (const FHitResult &CurrentHit : OutHits) {
      // check if we've hit a damageable actor
      ICombatDamageable *Damageable =
          Cast<ICombatDamageable>(CurrentHit.GetActor());

      if (Damageable) {
        // knock upwards and away from the impact normal
        const FVector Impulse =
            (CurrentHit.ImpactNormal * -MeleeKnockbackImpulse) +
            (FVector::UpVector * MeleeLaunchImpulse);

        // pass the damage event to the actor
        Damageable->ApplyDamage(MeleeDamage, this, CurrentHit.ImpactPoint,
                                Impulse);

        // call the BP handler to play effects, etc.
        DealtDamage(MeleeDamage, CurrentHit.ImpactPoint);
      }
    }
  }
}

void ARdGameCharacter::CheckCombo() {
  // are we playing a non-charge attack animation?
  if (bIsAttacking && !bIsChargingAttack) {
    UWorld *World = GetWorld();
    if (!World)
      return;

    // is the last attack input not stale?
    if (World->GetTimeSeconds() - CachedAttackInputTime <=
        ComboInputCacheTimeTolerance) {
      // consume the attack input so we don't accidentally trigger it twice
      CachedAttackInputTime = 0.0f;

      // increase the combo counter
      ++ComboCount;

      // do we still have a combo section to play?
      if (ComboCount < ComboSectionNames.Num()) {
        // notify enemies they are about to be attacked
        NotifyEnemiesOfIncomingAttack();

        // jump to the next combo section
        if (UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance()) {
          AnimInstance->Montage_JumpToSection(ComboSectionNames[ComboCount],
                                              ComboAttackMontage);
        }
      }
    }
  }
}

void ARdGameCharacter::CheckChargedAttack() {
  // raise the looped charged attack flag
  bHasLoopedChargedAttack = true;

  // jump to either the loop or the attack section depending on whether we're
  // still holding the charge button
  if (UAnimInstance *AnimInstance = GetMesh()->GetAnimInstance()) {
    AnimInstance->Montage_JumpToSection(bIsChargingAttack ? ChargeLoopSection
                                                          : ChargeAttackSection,
                                        ChargedAttackMontage);
  }
}

void ARdGameCharacter::NotifyEnemiesOfIncomingAttack() {
  // sweep for objects in front of the character to be hit by the attack
  TArray<FHitResult> OutHits;

  // start at the actor location, sweep forward
  const FVector TraceStart = GetActorLocation();
  const FVector TraceEnd =
      TraceStart + (GetActorForwardVector() * DangerTraceDistance);

  // check for pawn object types only
  FCollisionObjectQueryParams ObjectParams;
  ObjectParams.AddObjectTypesToQuery(ECC_Pawn);

  // use a sphere shape for the sweep
  FCollisionShape CollisionShape;
  CollisionShape.SetSphere(DangerTraceRadius);

  // ignore self
  FCollisionQueryParams QueryParams;
  QueryParams.AddIgnoredActor(this);

  if (GetWorld()->SweepMultiByObjectType(OutHits, TraceStart, TraceEnd,
                                         FQuat::Identity, ObjectParams,
                                         CollisionShape, QueryParams)) {
    // iterate over each object hit
    for (const FHitResult &CurrentHit : OutHits) {
      // check if we've hit a damageable actor
      ICombatDamageable *Damageable =
          Cast<ICombatDamageable>(CurrentHit.GetActor());

      if (Damageable) {
        // notify the enemy
        Damageable->NotifyDanger(GetActorLocation(), this);
      }
    }
  }
}

void ARdGameCharacter::ApplyDamage(float Damage, AActor *DamageCauser,
                                   const FVector &DamageLocation,
                                   const FVector &DamageImpulse) {
  // pass the damage event to the actor
  FDamageEvent DamageEvent;
  const float ActualDamage =
      TakeDamage(Damage, DamageEvent, nullptr, DamageCauser);

  // only process knockback and effects if we received nonzero damage
  if (ActualDamage > 0.0f) {
    // apply the knockback impulse
    GetCharacterMovement()->AddImpulse(DamageImpulse, true);

    // is the character ragdolling?
    if (GetMesh()->IsSimulatingPhysics()) {
      // apply an impulse to the ragdoll
      GetMesh()->AddImpulseAtLocation(DamageImpulse * GetMesh()->GetMass(),
                                      DamageLocation);
    }

    // pass control to BP to play effects, etc.
    ReceivedDamage(ActualDamage, DamageLocation, DamageImpulse.GetSafeNormal());
  }
}

void ARdGameCharacter::HandleDeath() {
  // disable movement while we're dead
  GetCharacterMovement()->DisableMovement();

  // enable full ragdoll physics
  GetMesh()->SetSimulatePhysics(true);

  // hide the life bar
  if (LifeBar) {
    LifeBar->SetHiddenInGame(true);
  }

  // pull back the camera
  GetCameraBoom()->TargetArmLength = DeathCameraDistance;

  // schedule respawning
  if (UWorld *World = GetWorld()) {
    World->GetTimerManager().SetTimer(RespawnTimer, this,
                                      &ARdGameCharacter::RespawnCharacter,
                                      RespawnTime, false);
  }
}

void ARdGameCharacter::ApplyHealing(float Healing, AActor *Healer) {
  // stub
}

void ARdGameCharacter::NotifyDanger(const FVector &DangerLocation,
                                    AActor *DangerSource) {
  // stub
}

void ARdGameCharacter::RespawnCharacter() {
  // destroy the character and let it be respawned by the Player Controller
  Destroy();
}

float ARdGameCharacter::TakeDamage(float Damage,
                                   struct FDamageEvent const &DamageEvent,
                                   AController *EventInstigator,
                                   AActor *DamageCauser) {
  // only process damage if the character is still alive
  if (CurrentHP <= 0.0f) {
    return 0.0f;
  }

  // reduce the current HP
  CurrentHP -= Damage;

  // have we run out of HP?
  if (CurrentHP <= 0.0f) {
    // die
    HandleDeath();
  } else {
    // update the life bar
    if (LifeBarWidget) {
      LifeBarWidget->SetLifePercentage(CurrentHP / MaxHP);
    }

    // enable partial ragdoll physics, but keep the pelvis vertical
    GetMesh()->SetPhysicsBlendWeight(0.5f);
    GetMesh()->SetBodySimulatePhysics(PelvisBoneName, false);
  }

  // return the received damage amount
  return Damage;
}

void ARdGameCharacter::Landed(const FHitResult &Hit) {
  Super::Landed(Hit);

  // is the character still alive?
  if (CurrentHP >= 0.0f) {
    // disable ragdoll physics
    GetMesh()->SetPhysicsBlendWeight(0.0f);
  }
}
