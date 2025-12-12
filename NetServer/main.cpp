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
    if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0)
    {
        std::cerr << "WSAStartup failed." << std::endl;
        return 1;
    }

    // 2. 리슨 소켓 생성
    SOCKET listenSock = socket(AF_INET, SOCK_STREAM, 0);
    if (listenSock == INVALID_SOCKET) {
        std::cerr << "Socket creation failed." << std::endl;
        WSACleanup();
        return 1;
    }

    SOCKADDR_IN serverAddr;
    memset(&serverAddr, 0, sizeof(serverAddr));
    serverAddr.sin_family = AF_INET;
    serverAddr.sin_addr.s_addr = htonl(INADDR_ANY);
    serverAddr.sin_port = htons(9000); // 9000번 포트

    if (bind(listenSock, (SOCKADDR*)&serverAddr, sizeof(serverAddr)) == SOCKET_ERROR) {
        std::cerr << "Bind failed." << std::endl;
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    if (listen(listenSock, SOMAXCONN) == SOCKET_ERROR) {
        std::cerr << "Listen failed." << std::endl;
        closesocket(listenSock);
        WSACleanup();
        return 1;
    }

    std::cout << "[Server] Listening on port 9000..." << std::endl;

    // 3. 연결 수락 루프
    while (true)
    {
        SOCKADDR_IN clientAddr;
        int addrLen = sizeof(clientAddr);
        SOCKET clientSock = accept(listenSock, (SOCKADDR*)&clientAddr, &addrLen);

        if (clientSock == INVALID_SOCKET) {
            std::cerr << "Accept failed." << std::endl;
            continue;
        }

        // Nagle 알고리즘 비활성화 (반응성 향상)
        BOOL opt = TRUE;
        setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (char*)&opt, sizeof(opt));

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

void ClientHandler(SOCKET clientSock, uint32_t sessionId)
{
    char buffer[1024]; // 수신 버퍼

    while (true)
    {
        // 1. 헤더 읽기 (패킷 크기를 알기 위해)
        int recvLen = recv(clientSock, buffer, sizeof(PacketHeader), 0);
        
        if (recvLen <= 0) break; // 연결 종료 또는 에러

        PacketHeader* header = (PacketHeader*)buffer;
        
        // 2. 패킷 내용물이 더 있다면 마저 읽기 (TCP 스트림 처리)
        // 주의: 실제 상용 서버에서는 recv가 요청한만큼 오지 않을 수 있으므로
        //      원하는 bodySize만큼 찰 때까지 루프를 돌며 recv해야 함.
        //      여기서는 간단한 예제로 한 번에 온다고 가정하거나, 블로킹 소켓 특성 활용.
        int bodySize = header->size - sizeof(PacketHeader);
        if (bodySize > 0)
        {
            int totalBodyRead = 0;
            while(totalBodyRead < bodySize)
            {
                int ret = recv(clientSock, buffer + sizeof(PacketHeader) + totalBodyRead, bodySize - totalBodyRead, 0);
                if(ret <= 0) {
                    recvLen = 0; // 루프 탈출 유도
                    break;
                }
                totalBodyRead += ret;
            }
        }
        
        if (recvLen <= 0) break;

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
            {
               // 공격 패킷 수신 및 브로드캐스트 예시
               // 실제로는 여기서 판정 로직이 들어갈 수 있음
               Pkt_Attack* pkt = (Pkt_Attack*)buffer;
               pkt->sessionId = sessionId;
               pkt->type = (uint16_t)PacketType::S2C_ATTACK_BROADCAST;
               
               BroadcastPacket(buffer, pkt->size, sessionId);
            }
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
