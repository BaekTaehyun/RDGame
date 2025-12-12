// Copyright 2024. bak1210. All Rights Reserved.

#include "GsNetworkWorker.h"
#include "HAL/RunnableThread.h"
#include "HAL/PlatformProcess.h"
#include "HAL/PlatformTime.h"

namespace GsNet
{
	FGsNetworkWorker::FGsNetworkWorker()
	{
		Session = new FGsSocketSession();
	}

	FGsNetworkWorker::~FGsNetworkWorker()
	{
		Shutdown();
		if (Session)
		{
			delete Session;
			Session = nullptr;
		}
	}

	bool FGsNetworkWorker::Init()
	{
		Session->Initialize();
		return true;
	}

	uint32 FGsNetworkWorker::Run()
	{
		double LastTime = FPlatformTime::Seconds();

		while (bRun)
		{
			double CurrentTime = FPlatformTime::Seconds();
			float DeltaTime = (float)(CurrentTime - LastTime);
			LastTime = CurrentTime;

			// 1. 명령 처리 (Process Commands)
			FWorkerCommand Cmd;
			while (CommandQueue.Dequeue(Cmd))
			{
				if (Cmd.Type == FWorkerCommand::EType::Connect)
				{
					// 이전 세션의 잔여 패킷 제거
					TArray<uint8> DummyPacket;
					while (RecvQueue.Dequeue(DummyPacket));

					ConnectionTryStartTime = CurrentTime;
					
					if (Session->Connect(Cmd.Ip, Cmd.Port))
					{
						bConnected = true; // 세션 활성화
						
						if (Session->GetState() == FGsSocketSession::ESessionState::Connecting)
						{
							bIsConnecting = true;
						}
						else
						{
							bIsConnecting = false;
						}
					}
					else
					{
						bConnected = false;
						bIsConnecting = false;
					}
				}
				else if (Cmd.Type == FWorkerCommand::EType::Disconnect)
				{
					Session->Disconnect();
					bConnected = false;
					bIsConnecting = false;
				}
			}

			// 2. I/O 및 상태 처리
			if (bConnected)
			{
				// 세션 상태 확인
				FGsSocketSession::ESessionState State = Session->GetState();

				if (State == FGsSocketSession::ESessionState::NotConnected)
				{
					bConnected = false;
					bIsConnecting = false;
					// 연결 끊김 처리 (필요 시 델리게이트 등)
				}
				else
				{
					// Recv (여기서 Connecting -> Connected 상태 변화가 일어날 수 있음)
					Session->TryRecv(RecvQueue);
					
					// Send
					Session->TrySend(SendQueue);

					// 상태 업데이트 후 다시 확인
					State = Session->GetState();

					if (bIsConnecting)
					{
						if (State == FGsSocketSession::ESessionState::Connected)
						{
							// 연결 성공!
							bIsConnecting = false;
							UE_LOG(LogTemp, Log, TEXT("Connection Established"));
						}
						else if (CurrentTime - ConnectionTryStartTime > CONNECTION_TIMEOUT)
						{
							UE_LOG(LogTemp, Warning, TEXT("Connection Timeout"));
							Session->Disconnect();
							bConnected = false;
							bIsConnecting = false;
						}
					}
					
					// 연결 상태 주기적 체크 (옵션)
					if (State == FGsSocketSession::ESessionState::Connected)
					{
						CheckConnection(DeltaTime);
					}
				}
			}

			// 3. 휴식 (Sleep) - CPU 점유율 방지
			FPlatformProcess::Sleep(0.001f); // 1ms
		}

		return 0;
	}

	void FGsNetworkWorker::CheckConnection(float DeltaTime)
	{
		// Ping-Pong 로직이나 주기적인 상태 확인을 여기에 구현
		// 예: 1초마다 연결 상태 확인 로그 또는 KeepAlive 패킷 전송
		/*
		LastConnectionCheckTime += DeltaTime;
		if (LastConnectionCheckTime > CONNECTION_CHECK_INTERVAL)
		{
			LastConnectionCheckTime = 0;
			// KeepAlive 패킷 전송 등
		}
		*/
	}

	void FGsNetworkWorker::Stop()
	{
		bRun = false;
	}

	void FGsNetworkWorker::Exit()
	{
		Session->Finalize();
	}

	void FGsNetworkWorker::Start()
	{
		if (Thread == nullptr)
		{
			bRun = true;
			Thread = FRunnableThread::Create(this, TEXT("GsNetworkWorker"), 0, TPri_Normal);
		}
	}

	void FGsNetworkWorker::Shutdown()
	{
		if (Thread)
		{
			Stop();
			Thread->WaitForCompletion();
			delete Thread;
			Thread = nullptr;
		}
	}

	void FGsNetworkWorker::Connect(const FString& Ip, int32 Port)
	{
		FWorkerCommand Cmd;
		Cmd.Type = FWorkerCommand::EType::Connect;
		Cmd.Ip = Ip;
		Cmd.Port = Port;
		CommandQueue.Enqueue(Cmd);
	}

	void FGsNetworkWorker::Disconnect()
	{
		FWorkerCommand Cmd;
		Cmd.Type = FWorkerCommand::EType::Disconnect;
		CommandQueue.Enqueue(Cmd);
	}

	void FGsNetworkWorker::EnqueueSendPacket(TArray<uint8>&& Packet)
	{
		SendQueue.Enqueue(MoveTemp(Packet));
	}

	bool FGsNetworkWorker::DequeueRecvPacket(TArray<uint8>& OutPacket)
	{
		return RecvQueue.Dequeue(OutPacket);
	}

	bool FGsNetworkWorker::IsConnected() const
	{
		return bConnected;
	}
}
