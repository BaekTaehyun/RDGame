// Copyright 2024. bak1210. All Rights Reserved.

#include "GsNetworkSubsystem.h"

void UGsNetworkSubsystem::Initialize(FSubsystemCollectionBase &Collection) {
  Super::Initialize(Collection);
  // 초기화 시에는 별도 워커를 생성하지 않음 (Connect 시 생성)
}

void UGsNetworkSubsystem::Deinitialize() {
  for (auto &Pair : Workers) {
    if (Pair.Value) {
      Pair.Value->Shutdown();
      Pair.Value.Reset();
    }
  }
  Workers.Empty();

  Super::Deinitialize();
}

void UGsNetworkSubsystem::Tick(float DeltaTime) {
  // 모든 활성 워커의 패킷 처리
  for (auto &Pair : Workers) {
    if (Pair.Value) {
      TArray<uint8> Packet;
      while (Pair.Value->DequeueRecvPacket(Packet)) {
        Dispatcher.Dispatch(Packet);
      }
    }
  }
}

TStatId UGsNetworkSubsystem::GetStatId() const {
  RETURN_QUICK_DECLARE_CYCLE_STAT(UGsNetworkSubsystem, STATGROUP_Tickables);
}

void UGsNetworkSubsystem::Connect(const FString &Ip, int32 Port,
                                  FName SessionName) {
  // 이미 존재하는 세션인지 확인
  TUniquePtr<GsNet::FGsNetworkWorker> *FoundWorker = Workers.Find(SessionName);

  if (!FoundWorker) {
    // 없으면 새로 생성
    TUniquePtr<GsNet::FGsNetworkWorker> NewWorker =
        MakeUnique<GsNet::FGsNetworkWorker>();
    NewWorker->Start();
    NewWorker->Connect(Ip, Port);
    Workers.Add(SessionName, MoveTemp(NewWorker));
  } else {
    // 있으면 재연결
    (*FoundWorker)->Connect(Ip, Port);
  }
}

void UGsNetworkSubsystem::Disconnect(FName SessionName) {
  if (TUniquePtr<GsNet::FGsNetworkWorker> *FoundWorker =
          Workers.Find(SessionName)) {
    (*FoundWorker)->Disconnect();

    // 옵션: 연결 끊으면 워커를 제거할지, 유지할지 결정.
    // 여기서는 자원 절약을 위해 제거하지 않고 유지하되, 필요 시 명시적 제거
    // 함수 추가 가능. 만약 완전히 제거하고 싶다면:
    // (*FoundWorker)->Shutdown();
    // Workers.Remove(SessionName);
  }
}

bool UGsNetworkSubsystem::IsConnected(FName SessionName) const {
  if (const TUniquePtr<GsNet::FGsNetworkWorker> *FoundWorker =
          Workers.Find(SessionName)) {
    return (*FoundWorker)->IsConnected();
  }
  return false;
}

void UGsNetworkSubsystem::Send(const TArray<uint8> &PacketData,
                               FName SessionName) {
  if (TUniquePtr<GsNet::FGsNetworkWorker> *FoundWorker =
          Workers.Find(SessionName)) {
    // 스레드로 전달하기 위해 데이터 복사
    TArray<uint8> DataCopy = PacketData;
    (*FoundWorker)->EnqueueSendPacket(MoveTemp(DataCopy));
  } else {
    // UE_LOG(LogTemp, Warning, TEXT("Send Failed: Session '%s' not found"),
    // *SessionName.ToString());
  }
}

void UGsNetworkSubsystem::UnregisterHandler(uint16 PacketId) {
  Dispatcher.UnregisterHandler(PacketId);
}
