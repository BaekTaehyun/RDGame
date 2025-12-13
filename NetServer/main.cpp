#include "Protocol.h"
#include "Session.h"
#include <WinSock2.h>
#include <iostream>
#include <map>
#include <memory>
#include <mutex>
#include <thread>

#pragma comment(lib, "ws2_32.lib")

void ClientHandler(SOCKET clientSock, uint32_t sessionId);
void BroadcastPacket(char *data, int len, uint32_t excludeId);

std::mutex g_sessionMutex;
std::map<uint32_t, std::shared_ptr<GsNet::ClientSession>> g_sessions;
uint32_t g_idCounter = 1;

int main() {
  if (sodium_init() < 0) {
    std::cerr << "[Server] libsodium initialization failed." << std::endl;
    return 1;
  }
  std::cout << "[Server] libsodium initialized." << std::endl;

  WSADATA wsaData;
  if (WSAStartup(MAKEWORD(2, 2), &wsaData) != 0) {
    std::cerr << "WSAStartup failed." << std::endl;
    return 1;
  }

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
  serverAddr.sin_port = htons(9000);

  if (bind(listenSock, (SOCKADDR *)&serverAddr, sizeof(serverAddr)) ==
      SOCKET_ERROR) {
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

  std::cout << "[Server] Listening on port 9000... (Encryption Enabled)"
            << std::endl;

  while (true) {
    SOCKADDR_IN clientAddr;
    int addrLen = sizeof(clientAddr);
    SOCKET clientSock = accept(listenSock, (SOCKADDR *)&clientAddr, &addrLen);

    if (clientSock == INVALID_SOCKET) {
      std::cerr << "Accept failed." << std::endl;
      continue;
    }

    BOOL opt = TRUE;
    setsockopt(clientSock, IPPROTO_TCP, TCP_NODELAY, (char *)&opt, sizeof(opt));

    auto session = std::make_shared<GsNet::ClientSession>();
    session->Socket = clientSock;

    if (!session->Initialize()) {
      std::cerr << "[Server] Session initialization failed." << std::endl;
      closesocket(clientSock);
      continue;
    }

    uint32_t newSessionId = 0;
    {
      std::lock_guard<std::mutex> lock(g_sessionMutex);
      newSessionId = g_idCounter++;
      session->SessionId = newSessionId;
      g_sessions[newSessionId] = session;
    }

    std::cout << "[Server] Client Connected. SessionID: " << newSessionId
              << std::endl;

    std::thread t(ClientHandler, clientSock, newSessionId);
    t.detach();
  }

  closesocket(listenSock);
  WSACleanup();
  return 0;
}

void ClientHandler(SOCKET clientSock, uint32_t sessionId) {
  std::shared_ptr<GsNet::ClientSession> session;
  {
    std::lock_guard<std::mutex> lock(g_sessionMutex);
    auto it = g_sessions.find(sessionId);
    if (it == g_sessions.end()) {
      closesocket(clientSock);
      return;
    }
    session = it->second;
  }

  // 1. Handshake
  if (!session->SendHandshake()) {
    std::cerr << "[Server] Failed to send handshake. SessionID: " << sessionId
              << std::endl;
    goto cleanup;
  }
  std::cout << "[Server] Handshake sent. SessionID: " << sessionId << std::endl;

  {
    int expectedSize = GsNet::ServerCrypto::GetHandshakePacketSize();
    uint8_t handshakeBuffer[64];
    int totalRecv = 0;

    while (totalRecv < expectedSize) {
      int recvLen = recv(clientSock, (char *)(handshakeBuffer + totalRecv),
                         expectedSize - totalRecv, 0);
      if (recvLen <= 0) {
        std::cerr << "[Server] Handshake recv failed. SessionID: " << sessionId
                  << std::endl;
        goto cleanup;
      }
      totalRecv += recvLen;
    }

    if (!session->ProcessHandshakeData(handshakeBuffer, expectedSize)) {
      std::cerr << "[Server] Handshake processing failed. SessionID: "
                << sessionId << std::endl;
      goto cleanup;
    }

    std::cout << "[Server] Handshake complete. SessionID: " << sessionId
              << " (Encrypted)" << std::endl;
  }

  // 2. Packet loop
  while (true) {
    // Strategy: Receive header first, decrypt it temporarily to get size,
    // then receive body, and decrypt entire packet together.
    // BUT this causes nonce mismatch.
    //
    // Better strategy: Always receive a fixed amount or use length prefix
    // before encryption. Since client encrypts entire packet at once, we need
    // to receive entire packet, but we don't know the size until we decrypt
    // header.
    //
    // SOLUTION: Make client send packet size BEFORE encrypted data (unencrypted
    // 2-byte prefix) OR: Read all available data, try to decrypt, check
    // validity
    //
    // SIMPLEST FIX: Have both client and server encrypt/decrypt header and body
    // SEPARATELY. This requires changing client to encrypt header first, then
    // body.
    //
    // FOR NOW: We'll receive the encrypted header (4 bytes), make a COPY and
    // decrypt ONLY the copy to peek at the size, then receive the rest, and
    // finally decrypt the ENTIRE original packet.

    uint8_t packetBuffer[4096];

    int headerRecv = 0;
    const int headerSize = sizeof(PacketHeader); // 4 bytes

    // 1. Receive Header (4 bytes)
    while (headerRecv < headerSize) {
      int recvLen = recv(clientSock, (char *)(packetBuffer + headerRecv),
                         headerSize - headerRecv, 0);
      if (recvLen <= 0)
        goto cleanup;
      headerRecv += recvLen;
    }

    // 2. Decrypt Header
    if (!session->Crypto.RecvXor(packetBuffer, headerSize)) {
      std::cerr << "[Server] Header decryption failed. SessionID: " << sessionId
                << std::endl;
      goto cleanup;
    }

    PacketHeader *header = (PacketHeader *)packetBuffer;

    if (header->size < headerSize || header->size > 4096) {
      std::cerr << "[Server] Invalid packet size: " << header->size
                << " SessionID: " << sessionId << std::endl;
      goto cleanup;
    }

    // 3. Receive Body
    int bodySize = header->size - headerSize;
    int bodyRecv = 0;

    if (bodySize > 0) {
      while (bodyRecv < bodySize) {
        int recvLen =
            recv(clientSock, (char *)(packetBuffer + headerSize + bodyRecv),
                 bodySize - bodyRecv, 0);
        if (recvLen <= 0)
          goto cleanup;
        bodyRecv += recvLen;
      }

      // 4. Decrypt Body
      if (!session->Crypto.RecvXor(packetBuffer + headerSize, bodySize)) {
        std::cerr << "[Server] Body decryption failed. SessionID: " << sessionId
                  << std::endl;
        goto cleanup;
      }
    }

    // Handle packet
    PacketType type = (PacketType)header->type;

    switch (type) {
    case PacketType::C2S_LOGIN_REQ: {
      Pkt_LoginReq *reqPkt = (Pkt_LoginReq *)packetBuffer;
      std::cout << "[Server] Login Request from SessionID: " << sessionId
                << " Username: " << reqPkt->username << std::endl;

      Pkt_LoginRes res;
      res.size = sizeof(Pkt_LoginRes);
      res.type = (uint16_t)PacketType::S2C_LOGIN_RES;
      res.mySessionId = sessionId;
      res.success = true;

      if (!session->SendEncrypted((char *)&res, res.size)) {
        std::cerr << "[Server] Failed to send login response. SessionID: "
                  << sessionId << std::endl;
        goto cleanup;
      }

      std::cout << "[Server] Login Response sent. SessionID: " << sessionId
                << std::endl;

      // [추가] 유저 입장 동기화
      {
        std::lock_guard<std::mutex> lock(g_sessionMutex);

        // 1. 새 유저 정보 패킷 생성
        Pkt_UserEnter newInfo;
        newInfo.size = sizeof(Pkt_UserEnter);
        newInfo.type = (uint16_t)PacketType::S2C_USER_ENTER;
        newInfo.sessionId = sessionId;
        newInfo.x = 0;
        newInfo.y = 0;
        newInfo.z = 0;
        newInfo.yaw = 0;

        // 2. 루프: 기존 유저 정보 <-> 새 유저 정보 교환
        for (auto &pair : g_sessions) {
          if (pair.first == sessionId)
            continue; // 나 자신 제외
          auto &otherSession = pair.second;
          if (otherSession && otherSession->bHandshakeComplete) {
            // A. 기존 유저들에게 "새 유저(나) 들어왔어" 알림
            otherSession->SendEncrypted((char *)&newInfo, newInfo.size);

            // B. 새 유저(나)에게 "기존 유저(너) 정보 줘" 알림
            Pkt_UserEnter existingInfo;
            existingInfo.size = sizeof(Pkt_UserEnter);
            existingInfo.type = (uint16_t)PacketType::S2C_USER_ENTER;
            existingInfo.sessionId = otherSession->SessionId;
            existingInfo.x = otherSession->LastX;
            existingInfo.y = otherSession->LastY;
            existingInfo.z = otherSession->LastZ;
            existingInfo.yaw = otherSession->LastYaw;

            session->SendEncrypted((char *)&existingInfo, existingInfo.size);
          }
        }
      }
    } break;

    case PacketType::C2S_MOVE_UPDATE: {
      Pkt_MoveUpdate *pkt = (Pkt_MoveUpdate *)packetBuffer;
      pkt->sessionId = sessionId; // 브로드캐스트를 위해 ID 채움
      pkt->type = (uint16_t)PacketType::S2C_MOVE_BROADCAST;

      // [추가] 마지막 위치 갱신 (추후 입장하는 유저를 위해)
      session->LastX = pkt->x;
      session->LastY = pkt->y;
      session->LastZ = pkt->z;
      session->LastYaw = pkt->yaw;

      BroadcastPacket((char *)packetBuffer, pkt->size, sessionId);
    } break;

    case PacketType::C2S_ATTACK: {
      Pkt_Attack *pkt = (Pkt_Attack *)packetBuffer;
      pkt->sessionId = sessionId;
      pkt->type = (uint16_t)PacketType::S2C_ATTACK_BROADCAST;
      BroadcastPacket((char *)packetBuffer, pkt->size, sessionId);
    } break;

    default:
      std::cerr << "[Server] Unknown packet type: " << (int)header->type
                << " SessionID: " << sessionId << std::endl;
      break;
    }
  }

  // [추가] 연결 종료 시 퇴장 알림 전송
  Pkt_UserLeave leavePkt;
  leavePkt.size = sizeof(Pkt_UserLeave);
  leavePkt.type = (uint16_t)PacketType::S2C_USER_LEAVE;
  leavePkt.sessionId = sessionId;
  BroadcastPacket((char *)&leavePkt, leavePkt.size, sessionId);

cleanup: {
  std::lock_guard<std::mutex> lock(g_sessionMutex);
  g_sessions.erase(sessionId);
}
  closesocket(clientSock);
  std::cout << "[Server] Client Disconnected. SessionID: " << sessionId
            << std::endl;
}

void BroadcastPacket(char *data, int len, uint32_t excludeId) {
  std::lock_guard<std::mutex> lock(g_sessionMutex);
  for (auto &pair : g_sessions) {
    if (pair.first == excludeId)
      continue;

    auto &session = pair.second;
    if (session && session->bHandshakeComplete) {
      session->SendEncrypted(data, len);
    }
  }
}
