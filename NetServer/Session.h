// Copyright 2024. bak1210. All Rights Reserved.
// Server-side Session Management with Encryption

#pragma once

#include "Crypto.h"
#include <WinSock2.h>
#include <cstdint>
#include <vector>

namespace GsNet {
// 수신 버퍼 상태
enum class RecvMode {
  Header, // 헤더(패킷 크기) 수신 중
  Body,   // 바디 수신 중
};

// 클라이언트 세션 정보
struct ClientSession {
  SOCKET Socket = INVALID_SOCKET;
  uint32_t SessionId = 0;
  ServerCrypto Crypto;

  // 수신 버퍼
  static constexpr int RECV_BUFFER_SIZE = 64 * 1024;
  uint8_t RecvBuffer[RECV_BUFFER_SIZE];
  int RecvOffset = 0;
  int ExpectedSize = 0;
  RecvMode Mode = RecvMode::Header;

  // 핸드셰이크 상태
  bool bHandshakeComplete = false;
  int HandshakeRecvOffset = 0;

  // 마지막 위치 (새 접속자 동기화용)
  float LastX = 0, LastY = 0, LastZ = 0;
  float LastYaw = 0;

  ClientSession() { memset(RecvBuffer, 0, RECV_BUFFER_SIZE); }

  // 수신 버퍼 초기화
  void ResetRecvBuffer() {
    RecvOffset = 0;
    ExpectedSize = 0;
    Mode = RecvMode::Header;
    memset(RecvBuffer, 0, RECV_BUFFER_SIZE);
  }

  // 세션 초기화
  bool Initialize() {
    if (!Crypto.Initialize()) {
      return false;
    }
    ResetRecvBuffer();
    bHandshakeComplete = false;
    HandshakeRecvOffset = 0;
    return true;
  }

  // 핸드셰이크 패킷 전송 (서버 PK + Nonce)
  bool SendHandshake() {
    int handshakeSize = ServerCrypto::GetHandshakePacketSize();
    uint8_t handshakeBuffer[crypto_kx_PUBLICKEYBYTES +
                            crypto_stream_chacha20_NONCEBYTES];

    // 패킷 구성: [PublicKey (32 bytes)][TxNonce (8 bytes)]
    memcpy(handshakeBuffer, Crypto.GetPk(), crypto_kx_PUBLICKEYBYTES);
    memcpy(handshakeBuffer + crypto_kx_PUBLICKEYBYTES, Crypto.GetTxNonce(),
           crypto_stream_chacha20_NONCEBYTES);

    int bytesSent = send(Socket, (char *)handshakeBuffer, handshakeSize, 0);
    return (bytesSent == handshakeSize);
  }

  // 클라이언트 핸드셰이크 수신 처리
  // 클라이언트가 별도의 핸드셰이크 응답을 보내지 않음 (클라이언트 코드 분석
  // 결과) 서버가 먼저 핸드셰이크를 보내고, 클라이언트는 이를 수신 후 암호화된
  // 패킷을 보냄 따라서 서버는 클라이언트의 공개키를 미리 알 수 없음
  //
  // 해결책: 클라이언트가 핸드셰이크 응답(PK + Nonce)을 보내도록 수정하거나,
  // 사전 공유 키(PSK) 방식을 사용
  //
  // 현재 클라이언트 코드를 보면 TODO 주석이 있음:
  // "// TODO: Send Handshake Packet back"
  // 이는 클라이언트가 핸드셰이크 응답을 보내야 함을 의미
  //
  // 따라서 서버에서는:
  // 1. 먼저 자신의 PK + Nonce 전송
  // 2. 클라이언트의 PK + Nonce 수신 대기
  // 3. 세션 키 생성 후 암호화 통신 시작

  // 클라이언트 핸드셰이크 데이터 처리 (PK + Nonce)
  bool ProcessHandshakeData(uint8_t *data, int len) {
    int expectedSize =
        crypto_kx_PUBLICKEYBYTES + crypto_stream_chacha20_NONCEBYTES;

    // 데이터가 충분한지 확인
    if (len < expectedSize) {
      return false; // 아직 더 받아야 함
    }

    // 클라이언트 공개키로 세션 키 생성
    uint8_t *clientPk = data;
    uint8_t *clientNonce = data + crypto_kx_PUBLICKEYBYTES;

    if (!Crypto.Handshake(clientPk)) {
      return false;
    }

    // 클라이언트의 Nonce 설정 (클라이언트 -> 서버 방향)
    Crypto.SetRxNonce(clientNonce);

    Crypto.SetHandshakeCompleted(true);
    bHandshakeComplete = true;

    return true;
  }

  // 암호화된 데이터 전송
  bool SendEncrypted(const char *data, int len) {
    if (!bHandshakeComplete) {
      return false;
    }

    // 데이터 복사 후 암호화
    std::vector<uint8_t> buffer(data, data + len);

    // 헤더(4바이트)와 바디 분리 암호화
    const int headerSize = 4;

    if (len >= headerSize) {
      // 1. Header Encryption -> Nonce++
      if (!Crypto.SendXor(buffer.data(), headerSize)) {
        return false;
      }

      // 2. Body Encryption -> Nonce++
      if (len > headerSize) {
        if (!Crypto.SendXor(buffer.data() + headerSize, len - headerSize)) {
          return false;
        }
      }
    } else {
      // 비정상 패킷, 일단 전체 암호화
      if (!Crypto.SendXor(buffer.data(), len)) {
        return false;
      }
    }

    int bytesSent = send(Socket, (char *)buffer.data(), len, 0);
    return (bytesSent == len);
  }

  // 복호화된 데이터 수신 (이미 RecvBuffer에 있는 데이터 복호화)
  bool DecryptRecvBuffer(int start, int len) {
    return Crypto.RecvXor(RecvBuffer + start, len);
  }
};
} // namespace GsNet
