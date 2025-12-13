#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "GameplayTagContainer.h"
#include "RdInputConfig.generated.h"

class UInputAction;

/**
 * GameplayTag ↔ InputAction 매핑 구조체
 */
USTRUCT(BlueprintType)
struct FRdInputAction {
  GENERATED_BODY()

public:
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly)
  TObjectPtr<const UInputAction> InputAction = nullptr;

  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (Categories = "InputTag"))
  FGameplayTag InputTag;
};

/**
 * 입력 설정 DataAsset
 * 에디터에서 Tag↔Action 매핑을 정의합니다.
 */
UCLASS(BlueprintType, Const)
class RDGAME_API URdInputConfig : public UDataAsset {
  GENERATED_BODY()

public:
  URdInputConfig(const FObjectInitializer &ObjectInitializer);

  /** Native 입력 (이동, 카메라) 액션 검색 */
  UFUNCTION(BlueprintCallable, Category = "Input")
  const UInputAction *
  FindNativeInputActionForTag(const FGameplayTag &InputTag,
                              bool bLogNotFound = true) const;

  /** Ability 입력 (공격, 스킬) 액션 검색 */
  UFUNCTION(BlueprintCallable, Category = "Input")
  const UInputAction *
  FindAbilityInputActionForTag(const FGameplayTag &InputTag,
                               bool bLogNotFound = true) const;

public:
  /** 기본 동작용 입력 액션 (이동, 카메라 등) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (TitleProperty = "InputAction"))
  TArray<FRdInputAction> NativeInputActions;

  /** 능력용 입력 액션 (공격, 스킬 등) */
  UPROPERTY(EditDefaultsOnly, BlueprintReadOnly,
            Meta = (TitleProperty = "InputAction"))
  TArray<FRdInputAction> AbilityInputActions;
};
