// Copyright 2024. bak1210. All Rights Reserved.
// Server-side Crypto Helper using libsodium

#pragma once

#include <cstdint>
#include <cstring>
#include <sodium.h>

namespace GsNet {
class ServerCrypto {
public:
  ServerCrypto() {
    memset(PublicKey, 0, crypto_kx_PUBLICKEYBYTES);
    memset(SecretKey, 0, crypto_kx_SECRETKEYBYTES);
    memset(RxKey, 0, crypto_stream_chacha20_KEYBYTES);
    memset(TxKey, 0, crypto_stream_chacha20_KEYBYTES);
    memset(RxNonce, 0, crypto_stream_chacha20_NONCEBYTES);
    memset(TxNonce, 0, crypto_stream_chacha20_NONCEBYTES);
  }

  // 초기화: 키 쌍 생성
  bool Initialize() {
    if (crypto_kx_keypair(PublicKey, SecretKey) != 0) {
      return false;
    }

    bHandshakeCompleted = false;
    memset(RxKey, 0, crypto_stream_chacha20_KEYBYTES);
    memset(TxKey, 0, crypto_stream_chacha20_KEYBYTES);
    memset(RxNonce, 0, crypto_stream_chacha20_NONCEBYTES);
    memset(TxNonce, 0, crypto_stream_chacha20_NONCEBYTES);

    // TxNonce 생성 (랜덤)
    randombytes_buf(TxNonce, crypto_stream_chacha20_NONCEBYTES);

    return true;
  }

  // 핸드셰이크: 클라이언트 공개키로 세션 키 생성
  // 서버는 crypto_kx_server_session_keys 사용
  bool Handshake(uint8_t *ClientPk) {
    // 서버 측: RxKey로 클라이언트 -> 서버, TxKey로 서버 -> 클라이언트
    if (crypto_kx_server_session_keys(RxKey, TxKey, PublicKey, SecretKey,
                                      ClientPk) != 0) {
      return false;
    }

    return true;
  }

  // 송신 암호화
  bool SendXor(uint8_t *Buf, int Len) {
    if (crypto_stream_chacha20_xor(Buf, Buf, Len, TxNonce, TxKey) != 0) {
      return false;
    }

    // Nonce 증가
    sodium_increment(TxNonce, crypto_stream_chacha20_NONCEBYTES);

    return true;
  }

  // 수신 복호화
  bool RecvXor(uint8_t *Buf, int Len) {
    if (crypto_stream_chacha20_xor(Buf, Buf, Len, RxNonce, RxKey) != 0) {
      return false;
    }

    // Nonce 증가
    sodium_increment(RxNonce, crypto_stream_chacha20_NONCEBYTES);

    return true;
  }

  void SetRxNonce(uint8_t *InRxNonce) {
    memcpy(RxNonce, InRxNonce, crypto_stream_chacha20_NONCEBYTES);
  }

  uint8_t *GetPk() { return PublicKey; }
  uint8_t *GetTxNonce() { return TxNonce; }

  bool IsHandshakeCompleted() const { return bHandshakeCompleted; }
  void SetHandshakeCompleted(bool bCompleted) {
    bHandshakeCompleted = bCompleted;
  }

  // 핸드셰이크 패킷 크기 (PublicKey + Nonce)
  static constexpr int GetHandshakePacketSize() {
    return crypto_kx_PUBLICKEYBYTES + crypto_stream_chacha20_NONCEBYTES;
  }

private:
  uint8_t PublicKey[crypto_kx_PUBLICKEYBYTES];
  uint8_t SecretKey[crypto_kx_SECRETKEYBYTES];
  uint8_t RxKey[crypto_stream_chacha20_KEYBYTES];
  uint8_t TxKey[crypto_stream_chacha20_KEYBYTES];
  uint8_t RxNonce[crypto_stream_chacha20_NONCEBYTES];
  uint8_t TxNonce[crypto_stream_chacha20_NONCEBYTES];

  bool bHandshakeCompleted = false;
};
} // namespace GsNet
