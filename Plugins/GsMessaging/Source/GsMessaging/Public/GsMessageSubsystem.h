// Copyright 2024. bak1210(백태현) All Rights Reserved.
// 
// 파일 목적:
// 메시지 핸들러들을 중앙에서 관리하는 서브시스템입니다.
// 핸들러의 동적 생성 및 생명주기 관리, 주기적인 업데이트(비동기 메시지 처리)를 담당합니다.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GsMessageHandler.h"
#include "GsMessageLog.h"
#include "Misc/ScopeRWLock.h"
#include "GsMessageSubsystem.generated.h"

/**
 * 메시지 시스템의 중앙 관리자
 * - 핸들러 자동 생성 및 캐싱
 * - 주기적 업데이트 (비동기 메시지 처리용)
 * - 스레드 안전한 접근 제공
 */
UCLASS()
class GSMESSAGING_API UGsMessageSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	// 서브시스템 접근 헬퍼
	static UGsMessageSubsystem* Get(const UObject* WorldContextObject = nullptr);

	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	// 주기적 업데이트 (TimerManager에 의해 호출됨)
	void Update(float DeltaTime);

	/**
	 * 핸들러 가져오기 (파라미터 없음)
	 * - 없으면 생성하여 반환
	 * - Thread-Safe (Double-Check Locking)
	 */
	template <typename TEnumId>
	TSharedPtr<TGsMessageHandler<TEnumId>> GetHandler()
	{
		static_assert(TIsEnum<TEnumId>::Value, "TEnumId must be an enum type");

		// 키 생성: Enum 이름 사용 (RTTI 의존성 제거)
		// UE5 Fix: TEnumId::StaticEnum() -> StaticEnum<TEnumId>()
		FString Key = StaticEnum<TEnumId>()->GetName();

		// 1차 확인 (Read Lock)
		{
			FRWScopeLock ScopeLock(HandlerMapLock, SLT_ReadOnly);
			if (TSharedPtr<IGsMessageHandler>* FoundHandler = HandlerMap.Find(Key))
			{
				return StaticCastSharedPtr<TGsMessageHandler<TEnumId>>(*FoundHandler);
			}
		}

		// 생성 필요 (Write Lock)
		{
			FRWScopeLock ScopeLock(HandlerMapLock, SLT_Write);
			
			// 락 획득 후 다시 확인 (Double-Check)
			if (TSharedPtr<IGsMessageHandler>* FoundHandler = HandlerMap.Find(Key))
			{
				return StaticCastSharedPtr<TGsMessageHandler<TEnumId>>(*FoundHandler);
			}

			// 새 핸들러 생성
			TSharedPtr<TGsMessageHandler<TEnumId>> NewHandler = MakeShared<TGsMessageHandler<TEnumId>>();
			
			// 로깅 설정 (자동 Traits 기반)
			NewHandler->SetTag(Key);
			NewHandler->SetLogger(&PrintTypeLog<TEnumId>);

			HandlerMap.Add(Key, NewHandler);
			return NewHandler;
		}
	}

	/**
	 * 핸들러 가져오기 (파라미터 1개)
	 * - 없으면 생성하여 반환
	 * - Thread-Safe (Double-Check Locking)
	 */
	template <typename TEnumId, typename TParam>
	TSharedPtr<TGsMessageHandlerOneParam<TEnumId, TParam>> GetHandler()
	{
		static_assert(TIsEnum<TEnumId>::Value, "TEnumId must be an enum type");

		// 키 생성: Enum 이름 사용 (RTTI 의존성 제거)
		// UE5 Fix: TEnumId::StaticEnum() -> StaticEnum<TEnumId>()
		FString Key = StaticEnum<TEnumId>()->GetName();

		// 1차 확인 (Read Lock)
		{
			FRWScopeLock ScopeLock(HandlerMapLock, SLT_ReadOnly);
			if (TSharedPtr<IGsMessageHandler>* FoundHandler = HandlerMap.Find(Key))
			{
				return StaticCastSharedPtr<TGsMessageHandlerOneParam<TEnumId, TParam>>(*FoundHandler);
			}
		}

		// 생성 필요 (Write Lock)
		{
			FRWScopeLock ScopeLock(HandlerMapLock, SLT_Write);

			// 락 획득 후 다시 확인 (Double-Check)
			if (TSharedPtr<IGsMessageHandler>* FoundHandler = HandlerMap.Find(Key))
			{
				return StaticCastSharedPtr<TGsMessageHandlerOneParam<TEnumId, TParam>>(*FoundHandler);
			}

			// 새 핸들러 생성
			TSharedPtr<TGsMessageHandlerOneParam<TEnumId, TParam>> NewHandler = MakeShared<TGsMessageHandlerOneParam<TEnumId, TParam>>();

			// 로깅 설정 (자동 Traits 기반)
			NewHandler->SetTag(Key);
			NewHandler->SetLogger(&PrintTypeLog<TEnumId>);

			HandlerMap.Add(Key, NewHandler);
			return NewHandler;
		}
	}

private:
	// 모든 핸들러를 관리하는 맵 (Key: EnumName)
	TMap<FString, TSharedPtr<IGsMessageHandler>> HandlerMap;
	
	// 맵 접근 보호를 위한 RWLock
	FRWLock HandlerMapLock;

	// 업데이트 타이머 핸들
	FTimerHandle UpdateTimerHandle;
};
