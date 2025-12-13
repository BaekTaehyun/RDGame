#pragma once

#include "CoreMinimal.h"
#include "NativeGameplayTags.h"

namespace RdGameplayTags {

// 태그 문자열로 검색하는 유틸리티 함수
RDGAME_API FGameplayTag FindTagByString(const FString &TagString,
                                        bool bMatchPartialString = false);

// ---------------------------------------------------------
// Input Tags - Native (이동, 카메라 등 기본 동작)
// ---------------------------------------------------------
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Move);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Mouse);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Look_Stick);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Jump);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Crouch);

// ---------------------------------------------------------
// Input Tags - Ability (공격, 스킬 등)
// ---------------------------------------------------------
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack_Combo);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Attack_Charged);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Camera_Toggle);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InputTag_Interact);

// ---------------------------------------------------------
// Init State Tags (Lyra 스타일 초기화 상태)
// ---------------------------------------------------------
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_Spawned);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataAvailable);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_DataInitialized);
RDGAME_API UE_DECLARE_GAMEPLAY_TAG_EXTERN(InitState_GameplayReady);

}; // namespace RdGameplayTags
