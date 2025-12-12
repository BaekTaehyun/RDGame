# 이동 동기화 분석 및 개선 제안 보고서

## 1. 현행 코드 분석 (`GsMovementNetReceiver.cpp`)

### 1.1 현재 구현 방식 (Dead Reckoning + Heuristic)
현재 코드는 **Dead Reckoning(추측 항법)**의 변형된 형태를 사용하고 있습니다. 패킷이 도착하면 `CurPos`와 `DestPos`정보를 기반으로 "예상 도착 지점(`considerPos`)"을 계산하여 이동시킵니다.

- **레이턴시 보정 시도**: 패킷 도착 시간과 현재 시간의 차이를 계산하여 `_latency` 값을 평활화(Smoothing) 하려고 시도합니다 (`Estimatelaytency`).
- **휴리스틱(직관적) 예측**: "방향 전환 정보를 늦게 받으므로, 이전 방향으로 더 이동했을 것이다"라는 가정 하에 `considerPos`를 계산합니다.
    - `considerPos = CurPos + NetDirection * Speed * (LatencyFactor)`
    - 90% 보정 등 매직 넘버를 사용하여 대략적인 오차를 줄이려 합니다.

### 1.2 문제점 (Issues)

1.  **부정확한 시간 동기화 (Time Synchronization)**
    - 주석(`// 테스트 해본결과 시간이 서버랑 오차가 생긴다..`)에서도 언급되었듯이, `inCurrentTime - inPacketTime` 계산은 서버와 클라이언트의 절대 시간이 완벽히 동기화되지 않으면 무의미하거나 오히려 해가 됩니다.
    - 음수 레이턴시가 발생할 수 있으며, 이로 인해 보정치가 튀어 캐릭터가 순간이동하거나 떨리는(Jitter) 현상이 발생합니다.

2.  **선형 보간의 한계 (Linear Interpolation Limitation)**
    - 단순히 "현재 위치 -> 예상 목적지"로 선형 이동합니다.
    - 급격한 방향 전환 시, 곡선이 아닌 각이 진 형태로 이동하여 부자연스럽습니다.

3.  **상태 버퍼 부재 (No State Buffer)**
    - 패킷이 도착하는 즉시 상태를 갱신합니다. 네트워킹 환경이 불안정하여 패킷이 뭉쳐서 오거나 순서가 뒤바뀔 경우(UDP), 캐릭터가 미친듯이 튀는 현상이 발생합니다.

## 2. 개선 제안 (Improvement Proposal)

보다 안정적인 동기화를 위해 **Entity Interpolation (엔티티 보간)** 기법을 도입을 제안합니다.

### 2.1 개념 (Concept)
- "현재 시간"이 아닌 **"과거 시간(Render Time)"**을 기준으로 캐릭터를 그립니다.
- 서버로부터 받은 상태(위치, 회전, 속도)를 **State Buffer**에 저장합니다.
- `RenderTime = CurrentTime - InterpolationDelay(보간 지연 시간, 보통 100~200ms)`
- 버퍼에서 `RenderTime` 바로 앞뒤의 두 상태(Snapshot)를 찾아 **보간(Interpolate)**합니다.

### 2.2 장점
- **극대화된 부드러움**: 패킷이 불규칙하게 도착해도 버퍼에서 꺼내 쓰므로 움직임이 끊기지 않습니다.
- **Jitter 방지**: 패킷 손실이나 지연이 있어도 `InterpolationDelay` 만큼의 여유 시간이 있어 자연스럽게 처리됩니다.

### 2.3 개선된 아키텍처 제안

```cpp
struct FMovementSnapshot
{
    double Timestamp;
    FVector Location;
    FVector Velocity;
    FRotator Rotation;
};

class FGsMovementNetReceiverImproved
{
private:
    // 상태 저장 버퍼 (Circular Buffer or Deque)
    TArray<FMovementSnapshot> _snapshotBuffer;
    
    // 보간 지연 시간 (예: 100ms)
    // 이 값이 클수록 부드럽지만 반응은 느려짐
    float _interpolationDelay = 0.1f; 

public:
    void ReceivePacket(const PKT_SC_MOVE* Packet)
    {
        // 1. 타임스탬프 보정 (NetworkTimeEstimation 필요)
        double correctedTimestamp = GetNetworkTime() - Packet->SendTime;
        
        // 2. 버퍼에 추가
        _snapshotBuffer.Add({correctedTimestamp, Packet->Pos, Packet->Vel, Packet->Rot});
        
        // 3. 오래된 스냅샷 정리
        PruneOldSnapshots();
    }

    void Update(float DeltaTime)
    {
        double renderTime = GetServerTime() - _interpolationDelay;
        
        // 1. RenderTime을 감싸는 두 스냅샷 찾기 (From, To)
        // 2. 비율(Alpha) 계산: (RenderTime - From.Time) / (To.Time - From.Time)
        // 3. 보간: Result = Lerp(From, To, Alpha)
        
        // * Hermite Spline을 사용하면 곡선 이동도 가능
        // Location = Hermite(From.Pos, From.Vel, To.Pos, To.Vel, Alpha)
    }
};
```

## 3. 핵심 변경 사항 요약

1.  **추측(Dead Reckoning) -> 보간(Interpolation) 전환**:
    - 예측 실패로 인한 위치 튐 현상을 제거하기 위해, 약간의 지연(Delay)을 감수하고 확실한 과거 데이터를 부드럽게 재생하는 방식으로 변경합니다.

2.  **서버 시간 추정 로직 (Clock Synchronization) 추가**:
    - 단순 `Current - PacketTime`이 아니라, 주기적인 Ping/Pong 또는 RTT 계산을 통해 `ServerTimeOffset`을 정확히 유지해야 합니다.

3.  **Hermite Spline (선택 사항)**:
    - 단순 선형 보간(Linear) 대신 위치와 **속도(Velocity)**를 모두 사용하는 곡선 보간을 적용하면 방향 전환이 훨씬 자연스러워집니다.
