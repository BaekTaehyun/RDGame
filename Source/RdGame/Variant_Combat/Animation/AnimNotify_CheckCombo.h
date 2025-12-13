// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "Animation/AnimNotifies/AnimNotify.h"
#include "CoreMinimal.h"

#include "AnimNotify_CheckCombo.generated.h"



/**
 *  AnimNotify to perform a combo string check.
 */
UCLASS()
class RDGAME_API UAnimNotify_CheckCombo : public UAnimNotify {
  GENERATED_BODY()

public:
  /** Perform the Anim Notify */
  virtual void Notify(USkeletalMeshComponent *MeshComp,
                      UAnimSequenceBase *Animation,
                      const FAnimNotifyEventReference &EventReference) override;

  /** Get the notify name */
  virtual FString GetNotifyName_Implementation() const override;
};
