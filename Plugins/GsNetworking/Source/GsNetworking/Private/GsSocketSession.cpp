// Copyright 2024. bak1210. All Rights Reserved.

#include "GsSocketSession.h"
#include "SocketSubsystem.h"
#include "Sockets.h"

namespace GsNet
{
	FGsSocketSession::FGsSocketSession()
	{
		RecvBuffer = new FGsRecvBuffer();
	}

	FGsSocketSession::~FGsSocketSession()
	{
		Finalize();
		if (RecvBuffer)
		{
			delete RecvBuffer;
			RecvBuffer = nullptr;
		}
	}

	bool FGsSocketSession::Initialize()
	{
		return true;
	}

	void FGsSocketSession::Finalize()
	{
		Disconnect();
	}

	bool FGsSocketSession::Connect(const FString& Ip, int32 Port)
	{
		// 이미 연결되어 있다면 끊고 다시 연결
		if (Socket)
		{
			Disconnect();
		}

		// 암호화 상태 초기화 (재연결 시 필수)
		Crypto.Initialize();

		if (!OpenSocket()) return false;

		TSharedRef<FInternetAddr> Addr = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateInternetAddr();
		bool bIsValid;
		Addr->SetIp(*Ip, bIsValid);
		Addr->SetPort(Port);

		if (!bIsValid)
		{
			UE_LOG(LogTemp, Error, TEXT("Invalid IP Address: %s"), *Ip);
			CloseSocket();
			return false;
		}

		// Non-blocking connect
		bool bConnected = Socket->Connect(*Addr);
		if (!bConnected)
		{
			ESocketErrors LastError = GetLastErrorCode();
			if (LastError != SE_EWOULDBLOCK && LastError != SE_EINPROGRESS)
			{
				UE_LOG(LogTemp, Error, TEXT("Connect Failed. Error: %d"), (int32)LastError);
				CloseSocket();
				return false;
			}
		}

		State = ESessionState::Connecting;
		
		// 버퍼 초기화
		PendingSendBuffer.Empty();
		PendingSendBytes = 0;
		RecvBuffer->Reset();

		return true;
	}

	void FGsSocketSession::Disconnect()
	{
		CloseSocket();
		PendingSendBuffer.Empty();
		PendingSendBytes = 0;
		State = ESessionState::NotConnected;
	}

	bool FGsSocketSession::IsConnected() const
	{
		return State == ESessionState::Connected;
	}

	bool FGsSocketSession::OpenSocket()
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		if (!SocketSubsystem) return false;

		Socket = SocketSubsystem->CreateSocket(NAME_Stream, Description, false);
		if (!Socket) return false;

		Socket->SetNonBlocking(true);
		Socket->SetReuseAddr(true);
		Socket->SetLinger(false, 0);
		int32 NewSize = 0;
		Socket->SetSendBufferSize(SEND_BUFFER_SIZE, NewSize);
		Socket->SetReceiveBufferSize(RECV_BUFFER_SIZE, NewSize);

		return true;
	}

	void FGsSocketSession::CloseSocket()
	{
		if (Socket)
		{
			Socket->Close();
			ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
			Socket = nullptr;
		}
	}

	ESocketErrors FGsSocketSession::GetLastErrorCode()
	{
		ISocketSubsystem* SocketSubsystem = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM);
		return SocketSubsystem ? SocketSubsystem->GetLastErrorCode() : SE_NO_ERROR;
	}

	bool FGsSocketSession::TryRecv(TQueue<TArray<uint8>, EQueueMode::Spsc>& OutRecvQueue)
	{
		if (!Socket) return false;

		// 연결 시도 중일 때 상태 체크
		if (State == ESessionState::Connecting)
		{
			ESocketConnectionState ConnState = Socket->GetConnectionState();
			if (ConnState == SCS_Connected)
			{
				State = ESessionState::Connected;
				// 연결 완료!
			}
			else if (ConnState == SCS_ConnectionError)
			{
				ESocketErrors Err = GetLastErrorCode();
				// SE_EBADF 는 아직 연결 중일 때 발생할 수 있음 (Legacy 코드 참조)
				if (Err != SE_EWOULDBLOCK && Err != SE_EINPROGRESS && Err != SE_EBADF)
				{
					UE_LOG(LogTemp, Error, TEXT("Connection Failed during TryRecv. Error: %d"), (int32)Err);
					Disconnect();
					return false;
				}
				// 아직 연결 안됨
				return true;
			}
			else
			{
				// SCS_NotConnected 상태
				return true;
			}
		}

		if (State != ESessionState::Connected) return false;

		uint32 PendingDataSize = 0;
		if (!Socket->HasPendingData(PendingDataSize) || PendingDataSize == 0)
		{
			return true;
		}

		int32 BytesRead = 0;
		
		// 핸드쉐이크 미완료 처리
		if (!Crypto.IsHandshakeCompleted())
		{
			int32 HandshakeSize = crypto_kx_PUBLICKEYBYTES + crypto_stream_chacha20_NONCEBYTES;
			if (PendingDataSize < (uint32)HandshakeSize) return true;

			if (Socket->Recv(RecvBuffer->Buffer, HandshakeSize, BytesRead))
			{
				// 핸드쉐이크 처리
				if (!Crypto.Handshake(RecvBuffer->Buffer))
				{
					UE_LOG(LogTemp, Error, TEXT("Handshake Failed"));
					Disconnect();
					return false;
				}

				Crypto.SetRxNonce(RecvBuffer->Buffer + crypto_kx_PUBLICKEYBYTES);
				
				// 클라이언트 핸드쉐이크 패킷 전송 (PK + Nonce)
				// 주의: 여기서는 즉시 전송하지 않고 큐에 넣거나 별도 처리 필요할 수 있음 
				// 구조상 TrySend 전에 넣는 것이 안전하나, 워커 스레드라면 직접 Send 호출 가능
				// 여기서는 단순화를 위해 생략하거나 별도 구현 필요.
				// TODO: Send Handshake Packet back
				
				Crypto.SetHandshakeCompleted(true);
				RecvBuffer->Reset();
			}
			else
			{
				ESocketErrors Err = GetLastErrorCode();
				if (Err != SE_EWOULDBLOCK && Err != SE_EINPROGRESS)
				{
					Disconnect();
					return false;
				}
			}
			return true;
		}

		// 일반 패킷 처리
		int32 BytesToRead = 0;
		if (RecvBuffer->RecvMode == FGsRecvBuffer::ERecvMode::Header)
		{
			BytesToRead = sizeof(uint16) - RecvBuffer->Offset;
		}
		else
		{
			BytesToRead = RecvBuffer->Size - RecvBuffer->Offset;
		}

		if (BytesToRead <= 0) return true;

		if (Socket->Recv(RecvBuffer->Buffer + RecvBuffer->Offset, BytesToRead, BytesRead))
		{
			if (BytesRead > 0)
			{
				// 복호화 실패 체크
				if (!Crypto.RecvXor(RecvBuffer->Buffer + RecvBuffer->Offset, BytesRead))
				{
					UE_LOG(LogTemp, Error, TEXT("RecvXor Failed"));
					Disconnect();
					return false;
				}

				RecvBuffer->Offset += BytesRead;

				if (RecvBuffer->RecvMode == FGsRecvBuffer::ERecvMode::Header)
				{
					if (RecvBuffer->Offset >= sizeof(uint16))
					{
						uint16 PacketSize = *((uint16*)RecvBuffer->Buffer);
						
						// 패킷 사이즈 검증
						if (PacketSize > RECV_BUFFER_SIZE || PacketSize < sizeof(uint16))
						{
							UE_LOG(LogTemp, Error, TEXT("Invalid Packet Size: %d"), PacketSize);
							Disconnect();
							return false;
						}

						RecvBuffer->Size = PacketSize;
						RecvBuffer->RecvMode = FGsRecvBuffer::ERecvMode::Body;
					}
				}
				else // Body
				{
					if (RecvBuffer->Offset >= RecvBuffer->Size)
					{
						// 패킷 완성
						TArray<uint8> PacketData;
						PacketData.Append(RecvBuffer->Buffer, RecvBuffer->Size);
						OutRecvQueue.Enqueue(MoveTemp(PacketData));

						RecvBuffer->Reset();
					}
				}
			}
		}
		else
		{
			ESocketErrors Err = GetLastErrorCode();
			if (Err != SE_EWOULDBLOCK && Err != SE_EINPROGRESS)
			{
				UE_LOG(LogTemp, Error, TEXT("Recv Failed Error: %d"), (int32)Err);
				Disconnect();
				return false;
			}
		}

		return true;
	}

	bool FGsSocketSession::TrySend(TQueue<TArray<uint8>, EQueueMode::Mpsc>& InSendQueue)
	{
		if (!Socket || State != ESessionState::Connected) return false;

		// 1. 이전에 보내지 못한 데이터가 있다면 먼저 처리
		if (PendingSendBuffer.Num() > 0)
		{
			int32 BytesSent = 0;
			int32 TotalToSend = PendingSendBuffer.Num() - PendingSendBytes;
			
			if (Socket->Send(PendingSendBuffer.GetData() + PendingSendBytes, TotalToSend, BytesSent))
			{
				PendingSendBytes += BytesSent;
				if (PendingSendBytes >= PendingSendBuffer.Num())
				{
					// 모두 전송 완료
					PendingSendBuffer.Empty();
					PendingSendBytes = 0;
				}
				else
				{
					// 일부만 전송됨 (다음 틱에 계속)
					return true;
				}
			}
			else
			{
				ESocketErrors Err = GetLastErrorCode();
				if (Err == SE_EWOULDBLOCK || Err == SE_EINPROGRESS)
				{
					return true; // 다음 틱에 다시 시도
				}
				else
				{
					UE_LOG(LogTemp, Error, TEXT("Send Failed (Pending) Error: %d"), (int32)Err);
					Disconnect();
					return false;
				}
			}
		}

		// 2. 큐에 있는 새로운 데이터 처리
		if (!InSendQueue.IsEmpty())
		{
			TArray<uint8> Packet;
			while (InSendQueue.Dequeue(Packet))
			{
				// 송신 버퍼 제한 체크 (PendingBuffer + NewPacket)
				if (PendingSendBuffer.Num() + Packet.Num() > MAX_SEND_BUFFER_PENDING)
				{
					UE_LOG(LogTemp, Error, TEXT("Send Buffer Overflow"));
					Disconnect();
					return false;
				}

				Crypto.SendXor(Packet.GetData(), Packet.Num());

				// 만약 PendingBuffer가 비어있다면 바로 전송 시도
				if (PendingSendBuffer.Num() == 0)
				{
					int32 BytesSent = 0;
					if (Socket->Send(Packet.GetData(), Packet.Num(), BytesSent))
					{
						if (BytesSent < Packet.Num())
						{
							// 일부만 전송됨 -> 나머지를 PendingBuffer에 저장
							int32 Remaining = Packet.Num() - BytesSent;
							PendingSendBuffer.Append(Packet.GetData() + BytesSent, Remaining);
							PendingSendBytes = 0;
						}
						// else: 모두 전송 완료
					}
					else
					{
						ESocketErrors Err = GetLastErrorCode();
						if (Err == SE_EWOULDBLOCK || Err == SE_EINPROGRESS)
						{
							// 블로킹 -> 전체를 PendingBuffer에 저장
							PendingSendBuffer.Append(Packet);
							PendingSendBytes = 0;
						}
						else
						{
							UE_LOG(LogTemp, Error, TEXT("Send Failed Error: %d"), (int32)Err);
							Disconnect();
							return false;
						}
					}
				}
				else
				{
					// 이미 PendingBuffer에 데이터가 있다면 순서를 위해 뒤에 추가만 함
					PendingSendBuffer.Append(Packet);
				}
			}
		}
		return true;
	}

	//-------------------------------------------------------------------------
	// FGsCrypto Implementation
	//-------------------------------------------------------------------------
	bool FGsCrypto::Initialize()
	{
		if (crypto_kx_keypair(PublicKey, SecretKey) != 0)
		{
			return false;
		}
		
		bHandshakeCompleted = false;
		FMemory::Memzero(RxKey, crypto_stream_chacha20_KEYBYTES);
		FMemory::Memzero(TxKey, crypto_stream_chacha20_KEYBYTES);
		FMemory::Memzero(RxNonce, crypto_stream_chacha20_NONCEBYTES);
		FMemory::Memzero(TxNonce, crypto_stream_chacha20_NONCEBYTES);
		
		return true;
	}

	bool FGsCrypto::Handshake(uint8* ServerPk)
	{
		if (crypto_kx_client_session_keys(RxKey, TxKey, PublicKey, SecretKey, ServerPk) != 0)
		{
			return false;
		}
		
		// TxNonce 생성 (랜덤)
		randombytes_buf(TxNonce, crypto_stream_chacha20_NONCEBYTES);
		
		return true;
	}

	bool FGsCrypto::SendXor(uint8* Buf, int32 Len)
	{
		if (crypto_stream_chacha20_xor(Buf, Buf, Len, TxNonce, TxKey) != 0)
		{
			return false;
		}
		
		// Nonce 증가
		sodium_increment(TxNonce, crypto_stream_chacha20_NONCEBYTES);
		
		return true;
	}

	bool FGsCrypto::RecvXor(uint8* Buf, int32 Len)
	{
		if (crypto_stream_chacha20_xor(Buf, Buf, Len, RxNonce, RxKey) != 0)
		{
			return false;
		}
		
		// Nonce 증가
		sodium_increment(RxNonce, crypto_stream_chacha20_NONCEBYTES);
		
		return true;
	}

	void FGsCrypto::SetRxNonce(uint8* InRxNonce)
	{
		FMemory::Memcpy(RxNonce, InRxNonce, crypto_stream_chacha20_NONCEBYTES);
	}
}
