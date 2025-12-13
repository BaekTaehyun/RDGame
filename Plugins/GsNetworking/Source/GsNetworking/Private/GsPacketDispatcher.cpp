// Copyright 2024. bak1210. All Rights Reserved.

#include "GsPacketDispatcher.h"

namespace GsNet {
void FGsPacketDispatcher::RegisterHandler(uint16 PacketId,
                                          FPacketHandlerDelegate Handler) {
  HandlerMap.Add(PacketId, Handler);
}

void FGsPacketDispatcher::UnregisterHandler(uint16 PacketId) {
  HandlerMap.Remove(PacketId);
}

void FGsPacketDispatcher::Dispatch(const TArray<uint8> &PacketData) {
  // 패킷 구조: [Size:2][Id:2][Body...]
  if (PacketData.Num() < 4)
    return;

  // 패킷 ID 읽기 (오프셋 2)
  uint16 PacketId = 0;
  FMemory::Memcpy(&PacketId, PacketData.GetData() + 2, sizeof(uint16));

  // 디버그 로그 삭제 (필요 시 주석 해제)
  // UE_LOG(LogTemp, Log, TEXT("[GsDispatcher] Dispatch PacketId: %d, Size:
  // %d"), PacketId, PacketData.Num());

  if (FPacketHandlerDelegate *Handler = HandlerMap.Find(PacketId)) {
    if (Handler->IsBound()) {
      Handler->Execute(PacketData);
    } else {
      UE_LOG(LogTemp, Warning,
             TEXT("[GsDispatcher] Handler Found but Unbound!"));
    }
  } else {
    UE_LOG(LogTemp, Warning,
           TEXT("[GsDispatcher] No Handler Found for PacketId: %d. MapNum: %d"),
           PacketId, HandlerMap.Num());
  }
}
} // namespace GsNet
