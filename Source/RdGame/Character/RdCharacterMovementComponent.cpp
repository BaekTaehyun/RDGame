
#include "RdCharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/Character.h"

URdCharacterMovementComponent::URdCharacterMovementComponent() {
  // Ensure we tick even if normally disabled (though for CMC default is true)
  PrimaryComponentTick.bCanEverTick = true;
  CurrentDriverMode = ENetworkDriverMode::UnrealUDP;
}

void URdCharacterMovementComponent::TickComponent(
    float DeltaTime, enum ELevelTick TickType,
    FActorComponentTickFunction *ThisTickFunction) {
  // If standard Unreal UDP mode (or local player), run default logic
  if (CurrentDriverMode == ENetworkDriverMode::UnrealUDP ||
      CurrentDriverMode == ENetworkDriverMode::None) {
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
    return;
  }

  // === Custom TCP Mode ===
  // We handle movement manually here purely for visualization/interpolation.
  // Physics are effectively bypassed.

  if (!PawnOwner || !UpdatedComponent) {
    return;
  }

  // 1. Interpolate Location
  FVector CurrentLoc = UpdatedComponent->GetComponentLocation();
  FVector NewLoc = FMath::VInterpTo(CurrentLoc, TargetLocation, DeltaTime,
                                    LocationInterpSpeed);

  // Check for teleport/snap if too far
  float DistSq = FVector::DistSquared(CurrentLoc, TargetLocation);
  if (DistSq > 500.0f * 500.0f) // > 5m
  {
    NewLoc = TargetLocation;
  }

  // 2. Interpolate Rotation
  FRotator CurrentRot = UpdatedComponent->GetComponentRotation();
  FRotator NewRot = FMath::RInterpTo(CurrentRot, TargetRotation, DeltaTime,
                                     RotationInterpSpeed);

  // Apply Transform
  // Use Teleport flag to avoid physics sweeping collisions that might block us
  UpdatedComponent->SetWorldLocationAndRotation(NewLoc, NewRot, false, nullptr,
                                                ETeleportType::TeleportPhysics);

  // 3. Set Velocity for Animation
  // IMPORTANT: We set this directly so the AnimBP sees the remote player's
  // speed
  Velocity = TargetVelocity;

  // Calculate Acceleration for AnimBP (usually checks if Acceleration != 0 to
  // determine "Intent") For remote players, we can simulate acceleration as a
  // vector in the direction of velocity. If velocity is small, acceleration is
  // zero.
  if (Velocity.SizeSquared() > 10.0f) {
    // Valid movement, set fake acceleration to allow AnimBP to "Run"
    Acceleration = Velocity.GetSafeNormal() * GetMaxAcceleration();
  } else {
    Acceleration = FVector::ZeroVector;
  }

  // Update derived data (Speed, etc)
  UpdateComponentVelocity();

  // 4. Update MovementMode for Animation (Flying vs Walking)
  // Simple raycast to check ground
  FVector Start = NewLoc;
  float CapsuleHalfHeight = 90.0f; // Default fallback
  if (ACharacter *Char = Cast<ACharacter>(PawnOwner)) {
    if (UCapsuleComponent *Cap = Char->GetCapsuleComponent()) {
      CapsuleHalfHeight = Cap->GetScaledCapsuleHalfHeight();
    }
  }

  FVector End = Start - FVector(0.0f, 0.0f, CapsuleHalfHeight + 10.0f);
  FHitResult Hit;
  FCollisionQueryParams Params;
  Params.AddIgnoredActor(PawnOwner);

  bool bHit = GetWorld()->LineTraceSingleByChannel(Hit, Start, End,
                                                   ECC_WorldStatic, Params);

  if (bHit) {
    SetMovementMode(MOVE_Walking);
  } else {
    SetMovementMode(MOVE_Falling);
  }
}

void URdCharacterMovementComponent::SetNetworkDriverMode(
    ENetworkDriverMode NewMode) {
  CurrentDriverMode = NewMode;

  if (CurrentDriverMode == ENetworkDriverMode::CustomTCP) {
    // Disable standard physics simulation to prevent conflict
    // However, we KEEP TickComponent enabled because we use our OWN custom tick
    // logic above. We do NOT call DisableMovement() because that stops Tick.

    // Reset physics state to avoid residual forces
    Velocity = FVector::ZeroVector;
    Acceleration = FVector::ZeroVector;

    // Ensure component can move freely
    SetMovementMode(MOVE_Custom, 0); // Use Custom mode to tell base class "I
                                     // got this" if we fall through
    // Actually, setting MOVE_Custom might be good, provided we don't call
    // Super::Tick which might try to process it. Since we gate Super::Tick,
    // state doesn't matter much to the base class, BUT AnimBP might check
    // "IsFalling" etc. So we should manually maintain Walking/Falling states as
    // done in Tick.
  } else {
    // Restore standard behavior
    SetMovementMode(MOVE_Walking);
  }
}

void URdCharacterMovementComponent::SetNetworkTarget(const FVector &NewLoc,
                                                     const FRotator &NewRot,
                                                     const FVector &NewVel) {
  TargetLocation = NewLoc;
  TargetRotation = NewRot;
  TargetVelocity = NewVel;
}
