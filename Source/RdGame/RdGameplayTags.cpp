#include "RdGameplayTags.h"
#include "GameplayTagsManager.h"

namespace RdGameplayTags {

// ---------------------------------------------------------
// Input Tags - Native
// ---------------------------------------------------------
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Move, "InputTag.Move", "이동 입력");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Mouse, "InputTag.Look.Mouse",
                               "마우스 시점 입력");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Look_Stick, "InputTag.Look.Stick",
                               "스틱 시점 입력");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Jump, "InputTag.Jump", "점프 입력");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Crouch, "InputTag.Crouch", "앉기 입력");

// ---------------------------------------------------------
// Input Tags - Ability
// ---------------------------------------------------------
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack, "InputTag.Attack", "기본 공격");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack_Combo, "InputTag.Attack.Combo",
                               "콤보 공격");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Attack_Charged,
                               "InputTag.Attack.Charged", "차지 공격");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Camera_Toggle, "InputTag.Camera.Toggle",
                               "카메라 전환");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InputTag_Interact, "InputTag.Interact",
                               "상호작용 입력");

// ---------------------------------------------------------
// Init State Tags
// ---------------------------------------------------------
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_Spawned, "InitState.Spawned",
                               "액터 스폰 완료");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataAvailable,
                               "InitState.DataAvailable",
                               "필요 데이터 사용 가능");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_DataInitialized,
                               "InitState.DataInitialized",
                               "데이터 초기화 완료");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(InitState_GameplayReady,
                               "InitState.GameplayReady",
                               "게임플레이 준비 완료");

// ---------------------------------------------------------
// Experience Tags
// ---------------------------------------------------------
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Experience_Type_Field, "Experience.Type.Field",
                               "필드 경험 (TCP)");
UE_DEFINE_GAMEPLAY_TAG_COMMENT(Experience_Type_Dungeon,
                               "Experience.Type.Dungeon", "던전 경험 (UDP)");

// ---------------------------------------------------------
// Utility Function
// ---------------------------------------------------------
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

} // namespace RdGameplayTags
