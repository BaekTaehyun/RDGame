// Copyright 2024. bak1210. All Rights Reserved.

#include "GsPacketDispatcher.h"

namespace GsNet
{
	void FGsPacketDispatcher::RegisterHandler(uint16 PacketId, FPacketHandlerDelegate Handler)
	{
		HandlerMap.Add(PacketId, Handler);
	}

	void FGsPacketDispatcher::UnregisterHandler(uint16 PacketId)
	{
		HandlerMap.Remove(PacketId);
	}

	void FGsPacketDispatcher::Dispatch(const TArray<uint8>& PacketData)
	{
		// 패킷 구조: [Size:2][Id:2][Body...]
		if (PacketData.Num() < 4) return;

		// 패킷 ID 읽기 (오프셋 2)
		uint16 PacketId = 0;
		FMemory::Memcpy(&PacketId, PacketData.GetData() + 2, sizeof(uint16));

		if (FPacketHandlerDelegate* Handler = HandlerMap.Find(PacketId))
		{
			Handler->ExecuteIfBound(PacketData);
		}
		else
		{
			// 처리되지 않은 패킷 ID 로그?
			// UE_LOG(LogTemp, Warning, TEXT("Unhandled Packet ID: %d"), PacketId);
		}
	}
}
