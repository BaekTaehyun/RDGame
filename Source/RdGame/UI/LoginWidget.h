// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "Blueprint/UserWidget.h"
#include "CoreMinimal.h"
#include "Network/Protocol.h"
#include "LoginWidget.generated.h"



class UEditableTextBox;
class UButton;
class UTextBlock;

/**
 * 서버 접속 및 로그인을 위한 위젯
 */
UCLASS()
class RDGAME_API ULoginWidget : public UUserWidget {
  GENERATED_BODY()

public:
  virtual void NativeConstruct() override;

protected:
  // UI 바인딩
  UPROPERTY(meta = (BindWidget))
  UEditableTextBox *Input_IP;

  UPROPERTY(meta = (BindWidget))
  UEditableTextBox *Input_Port;

  UPROPERTY(meta = (BindWidget))
  UEditableTextBox *Input_Username;

  UPROPERTY(meta = (BindWidget))
  UButton *Btn_Connect;

  UPROPERTY(meta = (BindWidget))
  UTextBlock *Text_Status;

private:
  UFUNCTION()
  void OnConnectClicked();

  // 패킷 핸들러
  void HandleLoginRes(const TArray<uint8> &PacketData);

  // 메인 스레드에서 UI 업데이트를 위한 헬퍼
  void UpdateStatus(const FString &Message,
                    FLinearColor Color = FLinearColor::White);

private:
  bool bIsWaitingForLogin = false;

  // 설정 저장/로드
  void SaveLoginSettings();
  void LoadLoginSettings();
  FString GetSettingsFilePath() const;
};
