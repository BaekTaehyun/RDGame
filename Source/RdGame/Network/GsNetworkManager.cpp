#include "Network/GsNetworkManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"
#include "GameFramework/Character.h"
#include "GsNetworkSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Network/GsNetworkMovementComponent.h"

void UGsNetworkManager::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
  UE_LOG(LogTemp, Log, TEXT("[GsNetworkManager] Initialize Called"));

  // BP 클래스 동적 로드
  // 경로: Content/ThirdPerson/Blueprints/BP_ThirdPersonCharacter.uasset
  FString ClassPath = TEXT("/Game/ThirdPerson/Blueprints/"
                           "BP_ThirdPersonCharacter.BP_ThirdPersonCharacter_C");
  RemoteActorClass = LoadClass<AActor>(nullptr, *ClassPath);

  if (!RemoteActorClass) {
    UE_LOG(LogTemp, Warning,
           TEXT("[GsNetworkManager] Failed to load BP_ThirdPersonCharacter. "
                "Using ACharacter instead."));
    RemoteActorClass = ACharacter::StaticClass();
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
      RemoteActors.Add(Pkt->sessionId, NewActor);
      UE_LOG(LogTemp, Log, TEXT("[GsNetworkManager] Spawned User %d at %s"),
             Pkt->sessionId, *SpawnLoc.ToString());

      if (APawn *NewPawn = Cast<APawn>(NewActor)) {
        NewPawn->SpawnDefaultController();
      }
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
