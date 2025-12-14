#include "GsNetworkManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GsNetworkMovementComponent.h"
#include "GsNetworkSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "RdNetworkSettings.h"
#include "RdRemoteCharacter.h"

void UGsNetworkManager::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
  UE_LOG(LogTemp, Log, TEXT("[GsNetworkManager] Initialize Called"));

  // 설정에서 RemoteCharacterClass 로드
  const URdNetworkSettings *Settings = GetDefault<URdNetworkSettings>();
  if (Settings && !Settings->RemoteCharacterClass.IsNull()) {
    RemoteActorClass = Settings->RemoteCharacterClass.LoadSynchronous();
  }

  // 로드 실패 시 기본 C++ 클래스 사용
  if (!RemoteActorClass) {
    UE_LOG(LogTemp, Warning,
           TEXT("[GsNetworkManager] Failed to load RemoteCharacterClass from "
                "Settings. Using default ARdRemoteCharacter."));
    RemoteActorClass = ARdRemoteCharacter::StaticClass();
  }

  // 핸들러 등록
  if (UGsNetworkSubsystem *NetworkSubsystem =
          Collection.InitializeDependency<UGsNetworkSubsystem>()) {
    NetworkSubsystem->RegisterHandler((uint16)PacketType::S2C_LOGIN_RES, this,
                                      &UGsNetworkManager::HandleLoginRes);
    NetworkSubsystem->RegisterHandler((uint16)PacketType::S2C_USER_ENTER, this,
                                      &UGsNetworkManager::HandleUserEnter);
    NetworkSubsystem->RegisterHandler((uint16)PacketType::S2C_USER_LEAVE, this,
                                      &UGsNetworkManager::HandleUserLeave);
    NetworkSubsystem->RegisterHandler((uint16)PacketType::S2C_MOVE_BROADCAST,
                                      this,
                                      &UGsNetworkManager::HandleMoveBroadcast);
  }
}

void UGsNetworkManager::Deinitialize() { Super::Deinitialize(); }

UGsNetworkManager *UGsNetworkManager::Get(const UObject *WorldContextObject) {
  if (!WorldContextObject)
    return nullptr;
  if (!GEngine)
    return nullptr;

  UWorld *World = GEngine->GetWorldFromContextObject(
      WorldContextObject, EGetWorldErrorMode::LogAndReturnNull);
  if (!World)
    return nullptr;

  return World->GetGameInstance()->GetSubsystem<UGsNetworkManager>();
}

void UGsNetworkManager::HandleLoginRes(const TArray<uint8> &Data) {
  if (Data.Num() < sizeof(Pkt_LoginRes))
    return;

  const Pkt_LoginRes *Pkt =
      reinterpret_cast<const Pkt_LoginRes *>(Data.GetData());

  if (Pkt->success) {
    MySessionId = Pkt->mySessionId;
    UE_LOG(LogTemp, Log,
           TEXT("[GsNetworkManager] Login Success. MySessionId: %d"),
           MySessionId);

    // 로그인 성공 알림
    if (OnLoginResult.IsBound()) {
      OnLoginResult.Broadcast(true);
    }
  } else {
    UE_LOG(LogTemp, Warning,
           TEXT("[GsNetworkManager] Login Failed (Success == false)"));
  }
}

void UGsNetworkManager::HandleUserEnter(const TArray<uint8> &Data) {
  if (Data.Num() < sizeof(Pkt_UserEnter))
    return;
  const Pkt_UserEnter *Pkt =
      reinterpret_cast<const Pkt_UserEnter *>(Data.GetData());

  // 1. 내꺼면 무시
  if (Pkt->sessionId == MySessionId)
    return;

  // 2. 이미 있으면 무시
  if (RemoteActors.Contains(Pkt->sessionId))
    return;

  // 3. 스폰
  UWorld *World = GetWorld();
  if (World) {
    FVector SpawnLoc(Pkt->x, Pkt->y, Pkt->z);
    FRotator SpawnRot(0, Pkt->yaw, 0);
    FActorSpawnParameters SpawnParams;
    SpawnParams.SpawnCollisionHandlingOverride =
        ESpawnActorCollisionHandlingMethod::AdjustIfPossibleButAlwaysSpawn;

    AActor *NewActor = World->SpawnActor<AActor>(RemoteActorClass, SpawnLoc,
                                                 SpawnRot, SpawnParams);
    if (NewActor) {
      if (ARdRemoteCharacter *RemoteChar = Cast<ARdRemoteCharacter>(NewActor)) {
        RemoteChar->SetSessionId(Pkt->sessionId);
        // 필드 스폰이므로 CustomTCP 모드 강제 설정
        RemoteChar->SetNetworkDriverMode(ENetworkDriverMode::CustomTCP);

        UE_LOG(LogTemp, Log,
               TEXT("[GsNetworkManager] Initialized RemoteCharacter %d "
                    "(CustomTCP Mode)"),
               Pkt->sessionId);
      }

      RemoteActors.Add(Pkt->sessionId, NewActor);
      UE_LOG(LogTemp, Log, TEXT("[GsNetworkManager] Spawned User %d at %s"),
             Pkt->sessionId, *SpawnLoc.ToString());

      // RemoteChar는 AI 컨트롤러가 필요 없음 (직접 보간)
    }
  }
}

void UGsNetworkManager::HandleUserLeave(const TArray<uint8> &Data) {
  if (Data.Num() < sizeof(Pkt_UserLeave))
    return;
  const Pkt_UserLeave *Pkt =
      reinterpret_cast<const Pkt_UserLeave *>(Data.GetData());

  if (AActor **FoundActor = RemoteActors.Find(Pkt->sessionId)) {
    if (*FoundActor) {
      (*FoundActor)->Destroy();
    }
    RemoteActors.Remove(Pkt->sessionId);
    UE_LOG(LogTemp, Log,
           TEXT("[GsNetworkManager] User %d Left. Destroyed Actor."),
           Pkt->sessionId);
  }
}

void UGsNetworkManager::HandleMoveBroadcast(const TArray<uint8> &Data) {
  if (Data.Num() < sizeof(Pkt_MoveUpdate))
    return;
  const Pkt_MoveUpdate *Pkt =
      reinterpret_cast<const Pkt_MoveUpdate *>(Data.GetData());

  if (Pkt->sessionId == MySessionId)
    return;

  if (AActor **FoundActor = RemoteActors.Find(Pkt->sessionId)) {
    AActor *Actor = *FoundActor;
    if (Actor) {
      FVector NewLoc(Pkt->x, Pkt->y, Pkt->z);
      FRotator NewRot(Pkt->pitch, Pkt->yaw, Pkt->roll);

      if (auto *MoveComp =
              Actor->FindComponentByClass<UGsNetworkMovementComponent>()) {
        FVector NewVel(Pkt->vx, Pkt->vy, Pkt->vz);
        MoveComp->OnNetworkDataReceived(NewLoc, NewRot, NewVel,
                                        (float)Pkt->timestamp);
      } else {
        Actor->SetActorLocation(NewLoc);
        Actor->SetActorRotation(NewRot);
      }
    }
  }
}

void UGsNetworkManager::ClearRemoteActors() {
  // When changing levels, all actors are destroyed by the Engine.
  // We just need to empty our list so we don't hold dangling pointers.
  RemoteActors.Empty();
  UE_LOG(LogTemp, Log, TEXT("[GsNetworkManager] RemoteActors map cleared."));
}
