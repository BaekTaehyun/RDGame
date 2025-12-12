// Copyright 2024. bak1210. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

// Libsodium include
#include "sodium.h"

namespace GsNet
{
	static constexpr int SEND_BUFFER_SIZE = 16 * 1024;
	static constexpr int RECV_BUFFER_SIZE = 64 * 1024;
	static constexpr int MAX_SEND_BUFFER_PENDING = 10 * 1024 * 1024; // 10MB

	// 연결 타임아웃 설정
	static constexpr float CONNECTION_TIMEOUT = 4.0f;
	static constexpr float CONNECTION_CHECK_INTERVAL = 1.0f;

	//-------------------------------------------------------------------------
	// 송신 버퍼 (Send Buffer)
	//-------------------------------------------------------------------------
	struct FGsSendBuffer
	{
		int32           Size = 0;
		int32           Offset = 0;
		uint8           Buffer[SEND_BUFFER_SIZE];

		void Reset()
		{
			Size = 0;
			Offset = 0;
			FMemory::Memzero(Buffer, SEND_BUFFER_SIZE);
		}
	};

	//-------------------------------------------------------------------------
	// 수신 버퍼 (Recv Buffer)
	//-------------------------------------------------------------------------
	struct FGsRecvBuffer
	{
		enum class ERecvMode
		{
			Header, // 헤더 수신 중
			Body,   // 바디 수신 중
		};

		int32           Size = 0;
		int32           Offset = 0;
		ERecvMode       RecvMode = ERecvMode::Header;
		uint8           Buffer[RECV_BUFFER_SIZE];

		void Reset()
		{
			Size = 0;
			Offset = 0;
			RecvMode = ERecvMode::Header;
			FMemory::Memzero(Buffer, RECV_BUFFER_SIZE);
		}
	};

	//-------------------------------------------------------------------------
	// 암호화 헬퍼 (Crypto Helper)
	//-------------------------------------------------------------------------
	class FGsCrypto
	{
	public:
		FGsCrypto() = default;
		~FGsCrypto() = default;

		bool Initialize();
		bool Handshake(uint8* ServerPk);
		bool SendXor(uint8* Buf, int32 Len);
		bool RecvXor(uint8* Buf, int32 Len);
		
		void SetRxNonce(uint8* RxNonce);
		uint8* GetPk() { return PublicKey; }
		uint8* GetTxNonce() { return TxNonce; }
		
		bool IsHandshakeCompleted() const { return bHandshakeCompleted; }
		void SetHandshakeCompleted(bool bCompleted) { bHandshakeCompleted = bCompleted; }

	private:
		uint8 PublicKey[crypto_kx_PUBLICKEYBYTES];
		uint8 SecretKey[crypto_kx_SECRETKEYBYTES];
		uint8 RxKey[crypto_stream_chacha20_KEYBYTES];
		uint8 TxKey[crypto_stream_chacha20_KEYBYTES];
		uint8 RxNonce[crypto_stream_chacha20_NONCEBYTES];
		uint8 TxNonce[crypto_stream_chacha20_NONCEBYTES];

		bool bHandshakeCompleted = false;
	};

	//-------------------------------------------------------------------------
	// 소켓 세션 (Socket Session)
	//-------------------------------------------------------------------------
	class FGsSocketSession
	{
	public:
		// 세션 상태 (Session State)
		enum class ESessionState
		{
			NotConnected,
			Connecting,
			Connected,
		};

		FGsSocketSession();
		virtual ~FGsSocketSession();

		bool Initialize();
		void Finalize();

		// 소켓 조작 (Socket Operations)
		bool Connect(const FString& Ip, int32 Port);
		void Disconnect();
		bool IsConnected() const;
		ESessionState GetState() const { return State; }

		// I/O 처리 (워커 스레드에서 호출됨)
		bool TryRecv(TQueue<TArray<uint8>, EQueueMode::Spsc>& OutRecvQueue);
		bool TrySend(TQueue<TArray<uint8>, EQueueMode::Mpsc>& InSendQueue);

		// Getter
		FString GetDescription() const { return Description; }

	private:
		bool OpenSocket();
		void CloseSocket();
		
		// 에러 처리 헬퍼
		ESocketErrors GetLastErrorCode();

	private:
		FSocket* Socket = nullptr;
		FString Description = TEXT("GsSocketSession");
		
		FGsRecvBuffer* RecvBuffer = nullptr;
		FGsCrypto Crypto;

		ESessionState State = ESessionState::NotConnected;

		// 전송 대기 중인 버퍼 (Pending Send Buffer)
		// EWOULDBLOCK 등으로 인해 전송되지 못한 데이터를 보관
		TArray<uint8> PendingSendBuffer;
		int32 PendingSendBytes = 0; // PendingSendBuffer 내에서 이미 전송된 바이트 수
	};
}
