#include "GsMessageSubsystem.h"
#include "Engine/World.h"
#include "TimerManager.h" // Reverted to TimerManager.h as Engine/TimerManager.h failed
#include "Kismet/GameplayStatics.h"

void UGsMessageSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().SetTimer(UpdateTimerHandle, [this]()
		{
			Update(0.0f);
		}, 0.016f, true);
	}
}

void UGsMessageSubsystem::Deinitialize()
{
	if (UWorld* World = GetWorld())
	{
		World->GetTimerManager().ClearTimer(UpdateTimerHandle);
	}

	// 잠금 상태에서 맵 초기화
	{
		FRWScopeLock ScopeLock(HandlerMapLock, SLT_Write);
		HandlerMap.Empty();
	}

	Super::Deinitialize();
}

void UGsMessageSubsystem::Update(float DeltaTime)
{
	// 실제 업데이트 호출 중 잠금을 피하고, 
	// 업데이트 중 핸들러가 새로운 핸들러를 추가할 경우 반복자 무효화를 방지하기 위해 
	// 핸들러 스냅샷을 생성합니다.
	TArray<TSharedPtr<IGsMessageHandler>> HandlersToUpdate;
	
	{
		FRWScopeLock ScopeLock(HandlerMapLock, SLT_ReadOnly);
		HandlerMap.GenerateValueArray(HandlersToUpdate);
	}

	// 스냅샷을 통해 안전하게 반복
	for (const auto& Handler : HandlersToUpdate)
	{
		if (Handler.IsValid())
		{
			Handler->Update();
		}
	}
}

UGsMessageSubsystem* UGsMessageSubsystem::Get(const UObject* WorldContextObject)
{
	if (UGameInstance* GI = UGameplayStatics::GetGameInstance(WorldContextObject))
	{
		return GI->GetSubsystem<UGsMessageSubsystem>();
	}
	return nullptr;
}
