// Copyright 2024. bak1210. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"

namespace GsNet
{
	DECLARE_DELEGATE_OneParam(FPacketHandlerDelegate, const TArray<uint8>&);

	class GSNETWORKING_API FGsPacketDispatcher
	{
	public:
		FGsPacketDispatcher() = default;
		~FGsPacketDispatcher() = default;

		void RegisterHandler(uint16 PacketId, FPacketHandlerDelegate Handler);
		void UnregisterHandler(uint16 PacketId);
		
		void Dispatch(const TArray<uint8>& PacketData);

	private:
		TMap<uint16, FPacketHandlerDelegate> HandlerMap;
	};
}
