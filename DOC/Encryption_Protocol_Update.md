# 암호화 프로토콜 변경 보고서: 헤더/바디 분리 암호화

## 1. 배경 및 문제점

### 1.1 초기 구현 방식 (단일 암호화 블록)
초기에는 패킷 전체(헤더 + 바디)를 하나의 블록으로 간주하여 `SendXor` 및 `RecvXor`를 수행했습니다.

*   **송신:** `Encrypt(Header + Body)` (Nonce 1 증가)
*   **수신:** `recv()`로 데이터를 받은 후 `Decrypt(Buffer)` (Nonce 1 증가)

### 1.2 문제점: TCP 스트림 특성과 Nonce 동기화 실패
TCP는 스트림 프로토콜이므로 패킷이 한 번에 도착한다는 보장이 없습니다. 패킷이 쪼개져서(Fragmentation) 도착하거나, 여러 패킷이 뭉쳐서 도착할 수 있습니다.

**시나리오:** 클라이언트가 100바이트 패킷을 보냄.
1.  **서버 수신 1:** 40바이트 도착.
2.  **서버 처리:** 40바이트에 대해 `RecvXor` 수행 -> **Nonce 1 증가**.
3.  **서버 수신 2:** 나머지 60바이트 도착.
4.  **서버 처리:** 60바이트에 대해 `RecvXor` 수행 -> **Nonce 또 1 증가**.

**결과:**
송신 측은 100바이트 전체에 대해 **Nonce N** 하나만 사용했지만, 수신 측은 앞부분 40바이트는 **Nonce N**으로 복호화하고, 뒷부분 60바이트는 **Nonce N+1**로 복호화하게 됩니다.
이로 인해 뒷부분 데이터의 복호화가 실패하여 쓰레기 값이 되며, 이후 모든 통신의 암호화가 깨지게 됩니다.

## 2. 해결 방안: 헤더(4 bytes)와 바디 분리 암호화

암호화 단위를 TCP 패킷 처리 단위와 일치시켜 동기화를 보장하는 방식을 채택했습니다.

### 2.1 프로토콜 구조
모든 패킷은 두 개의 암호화 블록으로 전송됩니다.

1.  **Block 1 (Header):** 고정 길이 **4바이트** (Packet Size + Packet Type). 독자적인 Nonce 사용.
2.  **Block 2 (Body):** 가변 길이 **(Packet Size - 4)바이트**. 독자적인 Nonce 사용.

### 2.2 동작 흐름

**[송신 측]**
```cpp
// 1. 헤더 암호화 (Nonce: N)
Crypto.SendXor(HeaderData, 4);

// 2. 바디 암호화 (Nonce: N+1)
if (BodySize > 0) {
    Crypto.SendXor(BodyData, BodySize);
}
```

**[수신 측]**
```cpp
// 1. 헤더 4바이트가 모일 때까지 수신 (Wait for 4 bytes)
recv(HeaderBuffer, 4);

// 2. 헤더 복호화 (Nonce: N)
Crypto.RecvXor(HeaderBuffer, 4);

// 3. 헤더에서 패킷 전체 크기(Size) 파싱
PacketSize = HeaderBuffer->size;

// 4. 나머지 바디 데이터가 모일 때까지 수신 (Wait for Size - 4 bytes)
recv(BodyBuffer, PacketSize - 4);

// 5. 바디 복호화 (Nonce: N+1)
Crypto.RecvXor(BodyBuffer, PacketSize - 4);
```

## 3. 구현 변경 사항

### 3.1 Client (Unreal Engine)
*   **`TrySend`:** 데이터를 보낼 때 항상 앞 4바이트를 먼저 `SendXor` 하고, 나머지를 이어서 `SendXor` 하도록 분리.
*   **`TryRecv`:** `RecvMode::Header` 상태에서 2바이트가 아닌 **4바이트**를 기준으로 읽도록 변경. 헤더가 완성되면 복호화 후 사이즈를 확인하고, `RecvMode::Body` 상태에서 바디가 완성되면 나머지 부분을 복호화.

### 3.2 Server (C++)
*   **`main.cpp` (수신):** `recv` 루프를 수정하여 먼저 4바이트를 확실히 읽고 복호화한 뒤, 파싱된 사이즈만큼 나머지를 읽고 복호화하도록 변경.
*   **`Session.h` (송신):** `SendEncrypted` 함수에서 헤더와 바디를 분리하여 두 번의 `SendXor` 호출로 변경.

## 4. 기대 효과
*   **안정성:** 패킷이 1바이트씩 잘려서 도착하더라도, 수신 측은 완벽한 블록(Header or Body)이 모일 때까지 복호화를 미루므로 Nonce 동기화가 깨지지 않습니다.
*   **확장성:** 헤더 크기가 고정(4바이트)되어 있어 처리가 명확하며, 추후 패킷 구조가 변경화더라도 대응이 용이합니다.
