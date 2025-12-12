// Copyright 2024. bak1210. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "GsNetworkWorker.h"
#include "GsPacketDispatcher.h"
#include "GsNetworkSubsystem.generated.h"

/**
 * 스레드 기반 네트워킹을 관리하는 서브시스템.
 */
UCLASS()
class GSNETWORKING_API UGsNetworkSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// FTickableGameObject 인터페이스
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override;
	virtual bool IsTickable() const override { return !IsTemplate(); }

	// 네트워크 API
	UFUNCTION(BlueprintCallable, Category = "GsNetworking")
	void Connect(const FString& Ip, int32 Port, FName SessionName = "Default");

	UFUNCTION(BlueprintCallable, Category = "GsNetworking")
	void Disconnect(FName SessionName = "Default");

	UFUNCTION(BlueprintCallable, Category = "GsNetworking")
	bool IsConnected(FName SessionName = "Default") const;

	// 원시 데이터 전송 (Send Raw Data)
	void Send(const TArray<uint8>& PacketData, FName SessionName = "Default");

	// 핸들러 등록 (Handler Registration)
	template<typename UserClass>
	void RegisterHandler(uint16 PacketId, UserClass* Object, void (UserClass::*Func)(const TArray<uint8>&))
	{
		Dispatcher.RegisterHandler(PacketId, GsNet::FPacketHandlerDelegate::CreateUObject(Object, Func));
	}
	
	void UnregisterHandler(uint16 PacketId);

private:
	// 세션 이름별 워커 관리
	TMap<FName, TUniquePtr<GsNet::FGsNetworkWorker>> Workers;
	GsNet::FGsPacketDispatcher Dispatcher;
};
