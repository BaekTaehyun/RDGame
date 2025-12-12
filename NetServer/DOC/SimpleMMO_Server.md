# Simple MMO Dedicated Server (TCP/IP)
이 프로젝트는 MMORPG의 이동 동기화(Dead Reckoning) 및 전투 동기화(Lag Compensation) 알고리즘을 테스트하기 위한 기초 C++ TCP 서버입니다.

## 🎯 프로젝트 목표
- **TCP 연결 수립**: 클라이언트 접속 처리.
- **간단한 인증(Auth)**: 접속한 클라이언트에게 ID(SessionID) 부여.
- **패킷 파싱**: TCP 스트림에서 Header와 Body를 분리하여 온전한 패킷으로 복원.
- **브로드캐스팅**: 한 클라이언트가 보낸 정보(이동/전투)를 다른 모든 클라이언트에게 전송.

## 🛠️ 개발 환경
- **Language**: C++17 이상
- **OS**: Windows (WinSock2 사용)
  - Linux/Mac으로 확장 시 `sys/socket`으로 교체 필요
- **Build System**: Visual Studio Solution 또는 CMake

---

## 📂 1. 프로토콜 설계 (Protocol.h)
TCP는 데이터 경계가 없으므로, 모든 패킷의 앞부분에 **헤더(Header)**를 붙여 패킷의 크기와 타입을 식별해야 합니다.

```cpp
#pragma once
#include <cstdint>

// 패킷 타입 정의
enum class PacketType : uint16_t
{
    NONE = 0,
    C2S_LOGIN_REQ = 1,      // 클라 -> 서버: 로그인 요청
    S2C_LOGIN_RES = 2,      // 서버 -> 클라: 로그인 결과 (내 ID 할당)
    C2S_MOVE_UPDATE = 3,    // 클라 -> 서버: 나 이동해요 (좌표, 속도)
    S2C_MOVE_BROADCAST = 4, // 서버 -> 클라: 쟤 이동한대 (브로드캐스팅)
    C2S_ATTACK = 5,         // 클라 -> 서버: 공격!
    S2C_ATTACK_BROADCAST = 6
};

#pragma pack(push, 1) // 바이트 정렬 (네트워크 전송용)

// 모든 패킷의 공통 헤더
struct PacketHeader
{
    uint16_t size; // 패킷 전체 크기 (헤더 포함)
    uint16_t type; // PacketType
};

// [로그인] 요청: 간단히 유저 이름만 전송
struct Pkt_LoginReq : public PacketHeader
{
    char username[32];
};

// [로그인] 응답: 서버가 부여한 SessionID 전송
struct Pkt_LoginRes : public PacketHeader
{
    uint32_t mySessionId;
    bool success;
};

// [이동] 데드 레코닝을 위한 데이터 구조
struct Pkt_MoveUpdate : public PacketHeader
{
    uint32_t sessionId; // 누가? (서버가 브로드캐스팅 할 때 채움)
    float x, y, z;      // 현재 위치 P_current
    float vx, vy, vz;   // 현재 속도 Velocity
    float yaw;          // 회전
    uint64_t timestamp; // 보낸 시간 (랙 보상용)
};

#pragma pack(pop)
```

## 💻 2. 서버 핵심 로직 구현

### A. 메인 서버 (main.cpp)
간단한 Select 모델 혹은 Blocking 소켓 + Thread 방식을 사용하여 이해하기 쉽게 구현합니다. 여기서는 1 클라이언트 = 1 스레드 구조로 프로토타이핑에 최적화합니다.

```cpp
#include <iostream>
#include <vector>
#include <thread>
#include <mutex>
#include <map>
#include <WinSock2.h>
#include "Protocol.h"

#pragma comment(lib, "ws2_32.lib")

// 전방 선언
void ClientHandler(SOCKET clientSock, uint32_t sessionId);
void BroadcastPacket(char* data, int len, uint32_t excludeId);

// 전역 변수 (동기화 필요)
std::mutex g_sessionMutex;
std::map<uint32_t, SOCKET> g_sessions; // SessionID -> Socket
uint32_t g_idCounter = 1;

int main()
{
    // 1. 윈속 초기화
    WSADATA wsaData;
    WSAStartup(MAKEWORD(2, 2), &wsaData);

    // 2. 리슨 소켓 생성
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(9000); // 9000번 포트

    bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr));
    listen(listenSock, SOMAXCONN);

    std::cout << "[Server] Listening on port 9000..." << std::endl;

    // 3. 연결 수락 루프
    while (true)
    {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrLen);

        if (clientSock == INVALID_SOCKET) continue;

        // 세션 ID 부여 및 관리 목록 추가
        uint32_t newSessionId = 0;
        {
            std::lock_guard<std::mutex> lock(g_sessionMutex);
            newSessionId = g_idCounter++;
            g_sessions[newSessionId] = clientSock;
        }

        std::cout << "[Server] Client Connected. SessionID: " << newSessionId << std::endl;

        // 클라이언트 처리를 위한 스레드 분리
        std::thread t(ClientHandler, clientSock, newSessionId);
        t.detach();
    }

    closesocket(listenSock);
    WSACleanup();
    return 0;
}
```

### B. 클라이언트 핸들러 & 브로드캐스팅
TCP 스트림 처리를 위해 "헤더 먼저 읽고 -> 사이즈만큼 바디 읽기" 패턴을 사용합니다.

```cpp
void ClientHandler(SOCKET clientSock, uint32_t sessionId)
{
    char buffer[1024]; // 수신 버퍼

    while (true)
    {
        // 1. 헤더 읽기 (패킷 크기를 알기 위해)
        int recvLen = recv(clientSock, buffer, sizeof(PacketHeader), 0);
        
        if (recvLen <= 0) break; // 연결 종료

        PacketHeader* header = (PacketHeader*)buffer;
        
        // 2. 패킷 내용물이 더 있다면 마저 읽기 (TCP 스트림 처리)
        int bodySize = header->size - sizeof(PacketHeader);
        if (bodySize > 0)
        {
            // 실제 구현에선 recv가 요청한 바이트를 다 못 읽을 수 있으므로 loop 돌려야 함
            recv(clientSock, buffer + sizeof(PacketHeader), bodySize, 0);
        }

        // 3. 패킷 핸들링
        PacketType type = (PacketType)header->type;

        switch (type)
        {
        case PacketType::C2S_LOGIN_REQ:
            {
                // 로그인 요청 처리 -> 응답 전송
                Pkt_LoginRes res;
                res.size = sizeof(Pkt_LoginRes);
                res.type = (uint16_t)PacketType::S2C_LOGIN_RES;
                res.mySessionId = sessionId; // 너의 ID는 이것이다.
                res.success = true;
                send(clientSock, (char*)&res, res.size, 0);
            }
            break;

        case PacketType::C2S_MOVE_UPDATE:
            {
                // 이동 패킷 수신
                Pkt_MoveUpdate* pkt = (Pkt_MoveUpdate*)buffer;
                
                // **중요**: 보낸 사람의 SessionID를 서버가 강제로 기입 (위조 방지)
                pkt->sessionId = sessionId; 
                pkt->type = (uint16_t)PacketType::S2C_MOVE_BROADCAST; // 타입 변경

                // 다른 모든 사람에게 브로드캐스팅
                BroadcastPacket(buffer, pkt->size, sessionId);
            }
            break;
            
        case PacketType::C2S_ATTACK:
            // 전투 패킷 처리 로직... (브로드캐스팅)
            break;
        }
    }

    // 연결 종료 처리
    {
        std::lock_guard<std::mutex> lock(g_sessionMutex);
        g_sessions.erase(sessionId);
    }
    closesocket(clientSock);
    std::cout << "[Server] Client Disconnected. SessionID: " << sessionId << std::endl;
}

// 나(excludeId)를 제외한 모두에게 전송
void BroadcastPacket(char* data, int len, uint32_t excludeId)
{
    std::lock_guard<std::mutex> lock(g_sessionMutex);
    for (auto& session : g_sessions)
    {
        if (session.first == excludeId) continue; // 나에게는 안 보냄 (Client-Side Prediction 때문)

        send(session.second, data, len, 0);
    }
}
```

---

## 🚀 3. 다음 단계: 알고리즘 적용 가이드
이 서버가 완성되었다면, 클라이언트(UE5/Unity 등)를 연결하여 다음 알고리즘을 테스트할 수 있습니다.

### Step 1. 데드 레코닝 (Dead Reckoning) 테스트
- **서버 역할**: `C2S_MOVE_UPDATE` 패킷을 받아서 그대로 다른 클라에게 뿌려줍니다.
- **클라이언트**:
  - 패킷을 받으면 `P_current`로 바로 이동시키지 않습니다.
  - Velocity 정보를 이용해 **추측(Extrapolation)**하고, 실제 위치와 차이가 나면 `Lerp`로 부드럽게 보정합니다.

### Step 2. 랙 보상 (Lag Compensation) 구현
- **서버 역할 추가**:
  - 서버는 `C2S_MOVE_UPDATE`를 받을 때마다, 메모리 상에 `Map<Time, Transform>` 형태의 히스토리 버퍼를 저장해야 합니다.
  - `C2S_ATTACK` 패킷이 오면, 패킷에 담긴 `timestamp`를 확인합니다.
  - 히스토리 버퍼에서 해당 시간의 적 캐릭터 위치를 꺼내와 충돌 처리를 계산합니다.

### Step 3. Nagle 알고리즘 끄기
`setsockopt`를 사용하여 `TCP_NODELAY` 옵션을 반드시 활성화하여 반응성을 높입니다.

```cpp
// 소켓 생성 직후 적용
BOOL opt = TRUE;
setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));
```
