
#pragma once

#include "CoreMinimal.h"
#include "Engine/DeveloperSettings.h"
#include "RdNetworkSettings.generated.h"

/**
 * 프로젝트 네트워크 관련 설정
 * Project Settings -> Game -> Rd Network 에서 설정 가능
 */
UCLASS(Config = Game, DefaultConfig,
       meta = (DisplayName = "Rd Network Settings"))
class RDGAME_API URdNetworkSettings : public UDeveloperSettings {
  GENERATED_BODY()

public:
  URdNetworkSettings();

  /** 타 유저(Remote Player)로 스폰할 캐릭터 클래스 (BP 지정 가능) */
  UPROPERTY(Config, EditAnywhere, Category = "Classes",
            meta = (MetaClass = "/Script/Engine.Character"))
  TSoftClassPtr<AActor> RemoteCharacterClass;

  /** 서버 접속 주소 (기본값) */
  UPROPERTY(Config, EditAnywhere, Category = "Connection")
  FString DefaultServerIP = TEXT("127.0.0.1");

  /** 서버 접속 포트 (기본값) */
  UPROPERTY(Config, EditAnywhere, Category = "Connection")
  int32 DefaultServerPort = 7777;
};
