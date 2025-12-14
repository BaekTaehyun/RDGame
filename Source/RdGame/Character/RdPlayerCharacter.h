// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "../RdGameCharacter.h"
#include "CoreMinimal.h"
#include "RdPlayerCharacter.generated.h"

class USpringArmComponent;
class UCameraComponent;
class URdHeroComponent;

/**
 * ARdPlayerCharacter
 *
 * The locally controlled player character.
 * Contains components and logic specific to the local player:
 * - Camera System (SpringArm, Camera)
 * - Input Handling (HeroComponent, Input Bindings)
 */
UCLASS()
class RDGAME_API ARdPlayerCharacter : public ARdGameCharacter {
  GENERATED_BODY()

public:
  ARdPlayerCharacter(const FObjectInitializer &ObjectInitializer);

  /** Camera boom positioning the camera behind the character */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components",
            meta = (AllowPrivateAccess = "true"))
  USpringArmComponent *CameraBoom;

  /** Follow camera */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components",
            meta = (AllowPrivateAccess = "true"))
  UCameraComponent *FollowCamera;

  /** Hero Component (Input Binding Manager) */
  UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Components",
            meta = (AllowPrivateAccess = "true"))
  URdHeroComponent *HeroComponent;

protected:
  /** Camera boom length while the character is dead */
  UPROPERTY(EditAnywhere, Category = "Camera",
            meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
  float DeathCameraDistance = 400.0f;

  /** Camera boom length when the character respawns */
  UPROPERTY(EditAnywhere, Category = "Camera",
            meta = (ClampMin = 0, ClampMax = 1000, Units = "cm"))
  float DefaultCameraDistance = 100.0f;

protected:
  virtual void BeginPlay() override;
  virtual void SetupPlayerInputComponent(
      class UInputComponent *PlayerInputComponent) override;

  // Input Handlers
public:
  virtual void DoMove(float Right, float Forward) override;
  virtual void DoLook(float Yaw, float Pitch) override;

  // Delegate Input to Base Logic
  virtual void DoJumpStart() override;
  virtual void DoJumpEnd() override;
  virtual void DoComboAttackStart() override;
  virtual void DoComboAttackEnd() override;
  virtual void DoChargedAttackStart() override;
  virtual void DoChargedAttackEnd() override;
  virtual void DoCameraToggle();

protected:
  // Overrides to handle Camera behavior during gameplay events
  virtual void HandleDeath() override;

  // Camera Toggle Implementation
  UFUNCTION(BlueprintImplementableEvent, Category = "Combat")
  void BP_ToggleCamera();

public:
  /** Returns CameraBoom subobject **/
  FORCEINLINE class USpringArmComponent *GetCameraBoom() const {
    return CameraBoom;
  }
  /** Returns FollowCamera subobject **/
  FORCEINLINE class UCameraComponent *GetFollowCamera() const {
    return FollowCamera;
  }
  /** Returns HeroComponent subobject **/
  FORCEINLINE class URdHeroComponent *GetHeroComponent() const {
    return HeroComponent;
  }
};
