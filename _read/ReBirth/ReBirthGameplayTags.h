// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "NativeGameplayTags.h"

namespace ReBirthGameplayTags {
REBIRTH_API FGameplayTag FindTagByString(const FString &TagString,
                                         bool bMatchPartialString = false);

// Input Tags - Native (Movement, Camera)
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Stick);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Crouch);

// Input Tags - Ability
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Skill);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);

// Init State Tags
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_Spawned);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataAvailable);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataInitialized);
REBIRTH_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_GameplayReady);
}; // namespace ReBirthGameplayTags
