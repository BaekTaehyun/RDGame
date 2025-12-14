
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "RdCharacterMovementComponent.generated.h"

/**
 * Network synchronization modes for this component.
 */
UENUM(BlueprintType)
enum class ENetworkDriverMode : uint8 {
  None,
  CustomTCP, // Sync via Custom TCP (Field)
  UnrealUDP  // Sync via Unreal Replication (Dungeon)
};

/**
 * URdCharacterMovementComponent
 *
 * Custom movement component that supports hybrid networking (TCP/UDP).
 * When in CustomTCP mode, it bypasses standard physics/networking and
 * directly interpolates based on received data, ensuring smooth animation
 * playback.
 */
UCLASS()
class RDGAME_API URdCharacterMovementComponent
    : public UCharacterMovementComponent {
  GENERATED_BODY()

public:
  URdCharacterMovementComponent();

  virtual void
  TickComponent(float DeltaTime, enum ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  /** Sets the current network driver mode */
  UFUNCTION(BlueprintCallable, Category = "Networking")
  void SetNetworkDriverMode(ENetworkDriverMode NewMode);

  /** Sets the target state received from the network (TCP) */
  void SetNetworkTarget(const FVector &NewLoc, const FRotator &NewRot,
                        const FVector &NewVel);

protected:
  /** Current network mode */
  UPROPERTY(VisibleAnywhere, Category = "Networking")
  ENetworkDriverMode CurrentDriverMode =
      ENetworkDriverMode::UnrealUDP; // Default to standard behavior

  /* TCP Interpolation Data */
  FVector TargetLocation;
  FRotator TargetRotation;
  FVector TargetVelocity;

  /* Interpolation Config */
  float LocationInterpSpeed = 10.0f;
  float RotationInterpSpeed = 10.0f;
};
