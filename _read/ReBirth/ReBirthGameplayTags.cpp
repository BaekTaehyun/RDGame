// Copyright Epic Games, Inc. All Rights Reserved.

#include "ReBirthGameplayTags.h"
#include "GameplayTagsManager.h"

namespace ReBirthGameplayTags {
// Input Tags - Native
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move",
                               "Move input action");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse",
                               "Look (mouse) input action");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Stick, "InputTag.Look.Stick",
                               "Look (stick) input action");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump",
                               "Jump input action");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "InputTag.Crouch",
                               "Crouch input action");

// Input Tags - Ability
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack",
                               "Attack input action");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Skill, "InputTag.Skill",
                               "Skill input action");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact",
                               "Interact input action");

// Init State Tags
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned",
                               "Actor has been spawned");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvailable,
                               "InitState.DataAvailable",
                               "All required data is available");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized,
                               "InitState.DataInitialized",
                               "All data has been initialized");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady,
                               "InitState.GameplayReady",
                               "Actor is ready for gameplay");

FGameplayTag FindTagByString(const FString &TagString,
                             bool bMatchPartialString) {
  const UGameplayTagsManager &Manager = UGameplayTagsManager::Get();
  FGameplayTag Tag = Manager.RequestGameplayTag(FName(*TagString), false);

  if (!Tag.IsValid() && bMatchPartialString) {
    FGameplayTagContainer AllTags;
    Manager.RequestAllGameplayTags(AllTags, true);

    for (const FGameplayTag &TestTag : AllTags) {
      if (TestTag.ToString().Contains(TagString)) {
        Tag = TestTag;
        break;
      }
    }
  }

  return Tag;
}
} // namespace ReBirthGameplayTags
