#pragma once

#include "Components/ActorComponent.h"
#include "CoreMinimal.h"
#include "Network/Protocol.h"
#include "GsNetworkMovementComponent.generated.h"


/**
 * 캐릭터의 움직임을 서버로 전송하는 컴포넌트
 * (위치, 속도, 회전 동기화)
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class RDGAME_API UGsNetworkMovementComponent : public UActorComponent {
  GENERATED_BODY()

public:
  UGsNetworkMovementComponent();

protected:
  virtual void BeginPlay() override;

public:
  virtual void
  TickComponent(float DeltaTime, ELevelTick TickType,
                FActorComponentTickFunction *ThisTickFunction) override;

  // 외부(GsNetworkManager)에서 데이터 넣어주는 함수
  void OnNetworkDataReceived(const FVector &NewLoc, const FRotator &NewRot,
                             const FVector &NewVel, float Timestamp);

private:
  // 현재 역할에 맞는 이동 전략 (Sender or Receiver)
  TSharedPtr<class IMovementStrategy> MovementStrategy;
};
