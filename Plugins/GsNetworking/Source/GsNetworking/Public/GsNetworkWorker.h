// Copyright 2024. bak1210. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "HAL/Runnable.h"
#include "Containers/Queue.h"
#include "GsSocketSession.h"

namespace GsNet
{
	class FGsNetworkWorker : public FRunnable
	{
	public:
		FGsNetworkWorker();
		virtual ~FGsNetworkWorker();

		// FRunnable 인터페이스
		virtual bool Init() override;
		virtual uint32 Run() override;
		virtual void Stop() override;
		virtual void Exit() override;

		// 제어 (Control)
		void Start();
		void Shutdown();

		// 게임 스레드용 API (API for Game Thread)
		void Connect(const FString& Ip, int32 Port);
		void Disconnect();
		void EnqueueSendPacket(TArray<uint8>&& Packet);
		bool DequeueRecvPacket(TArray<uint8>& OutPacket);

		bool IsConnected() const;

	private:
		void CheckConnection(float DeltaTime);

	private:
		FRunnableThread* Thread = nullptr;
		FGsSocketSession* Session = nullptr;
		
		// 스레드 안전성 (Thread Safety)
		FThreadSafeBool bRun = false;
		FThreadSafeBool bConnected = false;

		// 타임아웃 및 연결 체크
		double LastConnectionCheckTime = 0.0;
		double ConnectionTryStartTime = 0.0;
		bool bIsConnecting = false;

		// 명령 큐 (게임 스레드 -> 워커 스레드 요청)
		struct FWorkerCommand
		{
			enum class EType { Connect, Disconnect };
			EType Type;
			FString Ip;
			int32 Port;
		};
		TQueue<FWorkerCommand, EQueueMode::Mpsc> CommandQueue;

		// 데이터 큐
		TQueue<TArray<uint8>, EQueueMode::Mpsc> SendQueue;
		TQueue<TArray<uint8>, EQueueMode::Spsc> RecvQueue;
	};
}
