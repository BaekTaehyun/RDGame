// Copyright 2024. bak1210(백태현) All Rights Reserved.
// 
// 파일 목적:
// 메시지 로그 출력을 위한 기본 클래스와 Enum 타입별 로그 특성(Traits)을 정의합니다.
// 자동 로그 설정(On/Off) 및 포맷팅 기능을 제공합니다.

#pragma once

#include "CoreMinimal.h"
#include "UObject/Class.h"
#include "UObject/Package.h"
#include "UObject/ObjectMacros.h"
#include "Templates/Function.h"

// 로그 카테고리 선언 (다른 모듈에서 접근 가능하도록 API 매크로 추가)
GSMESSAGING_API DECLARE_LOG_CATEGORY_EXTERN(LogGsMessage, Log, All);

//-------------------------------------------------------------------------------------------------
// 템플릿 함수 포인터 정의 (Modern C++ TFunction으로 변경)
//-------------------------------------------------------------------------------------------------
template <typename Param1, typename Param2>
using fPtrVoidParam2Type = TFunction<void(Param1, Param2)>;

//-------------------------------------------------------------------------------------------------
// 로깅을 위한 Enum 타입 특성 (Traits)
//-------------------------------------------------------------------------------------------------
namespace EMessageLogType
{
	enum Type : uint8
	{
		LOG_NORMAL,
		LOG_WARNING,
		LOG_ERROR,
		LOG_SCREEN,
	};
}

template <typename TEnumState>
struct TGsTypeTraits
{
	static const EMessageLogType::Type	Type = EMessageLogType::LOG_NORMAL;
	static const bool					Enable = true;
	static const FString				TName;
};

// 기본 TName 구현
template <typename TEnumState>
const FString TGsTypeTraits<TEnumState>::TName = TEXT("Unknown");

// 매크로를 위한 헬퍼
template<typename T>
FString TypeString(FString ltype)
{
	return ltype;
}

// UE5 Fix: Added 'inline' to static member definition to prevent LNK2005 (multiple definition) errors
#define MESSAGE_LOG_TYPE(Target, LogType, flag)									\
template<>																		\
struct TGsTypeTraits<Target>													\
{																				\
	static const EMessageLogType::Type	Type	= EMessageLogType::LogType;		\
	static const bool					Enable	= flag;							\
	static const FString				TName;									\
};																				\
inline const FString TGsTypeTraits<Target>::TName = TypeString<Target>(#Target);

//-------------------------------------------------------------------------------------------------
// 로그 출력 구현
//-------------------------------------------------------------------------------------------------

// Enum을 문자열로 변환하는 헬퍼
template <typename T>
FString EnumToString(const T value, const FString& defaultValue)
{
	// UE5 Fix: Use StaticEnum<T>() directly instead of FindObject with ANY_PACKAGE
	// This is safer, faster, and removes the dependency on ANY_PACKAGE
	if (UEnum* pEnum = StaticEnum<T>())
	{
		return pEnum->GetNameStringByIndex((int32)value);
	}
	return defaultValue;
}

// 메인 로깅 함수
template<typename T>
void PrintTypeLog(T MessageId, FString& tagName)
{
	if (false == TGsTypeTraits<T>::Enable)
		return;

	// UE5 Fix: Simplified EnumToString call
	FString enumName = EnumToString<T>(MessageId, TEXT("Error"));

	switch (TGsTypeTraits<T>::Type)
	{
	case EMessageLogType::LOG_NORMAL:
		UE_LOG(LogGsMessage, Log, TEXT("%s ▬ [%s]::[%s]"), *tagName, *TGsTypeTraits<T>::TName, *enumName);
		break;
	case EMessageLogType::LOG_WARNING:
		UE_LOG(LogGsMessage, Warning, TEXT("%s ▬ [%s]::[%s]"), *tagName, *TGsTypeTraits<T>::TName, *enumName);
		break;
	case EMessageLogType::LOG_ERROR:
		UE_LOG(LogGsMessage, Error, TEXT("%s ▬ [%s]::[%s]"), *tagName, *TGsTypeTraits<T>::TName, *enumName);
		break;
	case EMessageLogType::LOG_SCREEN:
		if (GEngine) GEngine->AddOnScreenDebugMessage(-1, 5.f, FColor::Red, FString::Printf(TEXT("%s ▬ [%s]::[%s]"), *tagName, *TGsTypeTraits<T>::TName, *enumName));
		break;
	}
}

//-------------------------------------------------------------------------------------------------
// 메시지 출력 기본 클래스
//-------------------------------------------------------------------------------------------------
template<typename TMessageId>
class MessageHandlerLog
{
protected:
	FString											_preTag;
	fPtrVoidParam2Type<TMessageId, FString&>		_printFn = nullptr;

public:
	MessageHandlerLog() = default;
	virtual ~MessageHandlerLog() = default;

public:
	void SetTag(const FString& inTag)								{ _preTag = inTag; }
	void SetLogger(fPtrVoidParam2Type<TMessageId, FString&> inFn)	{ _printFn = inFn; }

protected:
	void Log(TMessageId id)
	{
		// UE5 Fix: TFunction comparison with nullptr is ambiguous/unsupported in some contexts.
		// Use boolean conversion instead.
		if (_printFn)
		{
			_printFn(id, _preTag);
		}
	}
};
