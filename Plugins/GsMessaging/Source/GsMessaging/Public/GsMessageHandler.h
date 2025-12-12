// Copyright 2024. bak1210(백태현) All Rights Reserved.
// 
// 파일 목적:
// 메시지 델리게이트 관리 및 비동기 처리를 담당하는 핸들러 클래스들을 정의합니다.
// 스레드 안전한 큐와 델리게이트 래퍼 함수들을 제공합니다.

#pragma once

#include "CoreMinimal.h"
#include "GsMessageLog.h"
#include "GsMessage.h"
#include "Containers/Queue.h"
#include "Misc/ScopeRWLock.h"
#include <type_traits> // std::conditional, std::is_rvalue_reference, std::remove_reference

// 서브시스템에 의한 일반적 관리를 위한 인터페이스
class IGsMessageHandler
{
public:
	virtual ~IGsMessageHandler() = default;
	virtual void Update() = 0;
};

//------------------------------------------------------------------------------
// 메시지 핸들러 (파라미터 없음)
//------------------------------------------------------------------------------
template <typename TMessageId>
class TGsMessageHandler final : public MessageHandlerLog<TMessageId>, public IGsMessageHandler
{
	static_assert(TIsEnum<TMessageId>::Value, "TMessageId is not enumerator type");
	DECLARE_EVENT(TGsMessageHandler, MessageDelegator);

private:
	TMap<TMessageId, MessageDelegator>				_router;
	TArray<TMessageId>								_asyncMessage;
	FRWLock											_asyncRWLock;

public:
	TGsMessageHandler() : MessageHandlerLog<TMessageId>() { }
	virtual ~TGsMessageHandler() { RemoveAll(); }

	void RemoveAll() { _router.Empty(); }

	// --- 등록 래퍼 함수들 ---
	template <typename UserClass, typename... ParamTypes, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddRaw(TMessageId MessageId, UserClass* InUserObject, typename TMemFunPtrType<false, UserClass, void(ParamTypes..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddRaw(InUserObject, InFunc, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename UserClass, typename... ParamTypes, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddUObject(TMessageId MessageId, UserClass* InUserObject, typename TMemFunPtrType<false, UserClass, void(ParamTypes..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddUObject(InUserObject, InFunc, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}
	
	template <typename FunctorType, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddLambda(TMessageId MessageId, FunctorType&& InFunctor, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddLambda(InFunctor, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename UObjectTemplate, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddUFunction(TMessageId MessageId, UObjectTemplate* InUserObject, const FName& InFunctionName, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddUFunction(InUserObject, InFunctionName, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename... ParamTypes, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddStatic(TMessageId MessageId, typename TBaseStaticDelegateInstance<void(ParamTypes...), FDefaultDelegateUserPolicy, VarTypes...>::FFuncPtr InFunc, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddStatic(InFunc, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename UserClass, typename FunctorType, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddWeakLambda(TMessageId MessageId, UserClass* InUserObject, FunctorType&& InFunctor, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddWeakLambda(InUserObject, InFunctor, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename UserClass, typename... ParamTypes, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddSP(TMessageId MessageId, const TSharedRef<UserClass, ESPMode::ThreadSafe>& InUserObjectRef, typename TMemFunPtrType<false, UserClass, void(ParamTypes..., VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddSP(InUserObjectRef, InFunc, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	void Remove(const TPair<TMessageId, FDelegateHandle>& Handle)
	{
		if (!Handle.Value.IsValid() || _router.Num() == 0) return;

		if (MessageDelegator* delegateFunc = _router.Find(Handle.Key))
		{
			// UE5 Fix: RemoveDelegateInstance is private, use Remove instead
			delegateFunc->Remove(Handle.Value);
		}
	}

	virtual void SendMessage(TMessageId MessageId)
	{
		if (const MessageDelegator* delegateFunc = _router.Find(MessageId))
		{
			MessageHandlerLog<TMessageId>::Log(MessageId);
			if (delegateFunc->IsBound())
			{
				delegateFunc->Broadcast();
			}
		}
	}

	virtual void SendMessageAsync(TMessageId MessageId)
	{
		FRWScopeLock ScopeWriteLock(_asyncRWLock, SLT_Write);
		_asyncMessage.Add(MessageId);
	}

	virtual void Update() override
	{
		TArray<TMessageId> copyQueue;
		{
			FRWScopeLock ScopeWriteLock(_asyncRWLock, SLT_Write);
			Swap(copyQueue, _asyncMessage);
		}

		for (const TMessageId& messageId : copyQueue)
		{
			SendMessage(messageId);
		}
	}
};

//------------------------------------------------------------------------------
// 메시지 핸들러 (파라미터 1개)
//------------------------------------------------------------------------------
template <typename TMessageId, typename TParam>
class TGsMessageHandlerOneParam final : public MessageHandlerLog<TMessageId>, public IGsMessageHandler
{
	static_assert(TIsEnum<TMessageId>::Value, "TMessageId is not enumerator type");
	
	// UE5/Modern C++ Fix: Use std traits instead of Unreal legacy traits
	using TParamStoreType = typename std::conditional<std::is_rvalue_reference<TParam>::value, typename std::remove_reference<TParam>::type, TParam>::type;
	using TParamInvokeType = typename std::conditional<std::is_rvalue_reference<TParam>::value, typename std::add_lvalue_reference<typename std::remove_reference<TParam>::type>::type, TParam>::type;

public:
	DECLARE_EVENT_OneParam(TGsMessageHandlerOneParam, MessageDelegator, TParamInvokeType);

private:
	TMap<TMessageId, MessageDelegator>					_router;
	TArray<TGsMessage<TMessageId, TParamStoreType>>		_asyncMessageBufferA;
	TArray<TGsMessage<TMessageId, TParamStoreType>>		_asyncMessageBufferB;
	TArray<TGsMessage<TMessageId, TParamStoreType>>*	_asyncMessage;
	FRWLock												_asyncRWLock;

public:
	TGsMessageHandlerOneParam()
		: MessageHandlerLog<TMessageId>()
		, _asyncMessage(&_asyncMessageBufferA)
	{
	}
	virtual ~TGsMessageHandlerOneParam() { RemoveAll(); }

	virtual void RemoveAll() { _router.Empty(); }

	// --- 등록 래퍼 함수들 ---
	template <typename UserClass, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddRaw(TMessageId MessageId, UserClass* InUserObject, typename TMemFunPtrType<false, UserClass, void(TParamInvokeType, VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddRaw(InUserObject, InFunc, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename UserClass, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddUObject(TMessageId MessageId, UserClass* InUserObject, typename TMemFunPtrType<false, UserClass, void(TParamInvokeType, VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddUObject(InUserObject, InFunc, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template<typename FunctorType, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddLambda(TMessageId MessageId, FunctorType&& InFunctor, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddLambda(InFunctor, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename UObjectTemplate, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddUFunction(TMessageId MessageId, UObjectTemplate* InUserObject, const FName& InFunctionName, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddUFunction(InUserObject, InFunctionName, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddStatic(TMessageId MessageId, typename TBaseStaticDelegateInstance<void(TParamInvokeType), FDefaultDelegateUserPolicy, VarTypes...>::FFuncPtr InFunc, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddStatic(InFunc, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename UserClass, typename FunctorType, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddWeakLambda(TMessageId MessageId, UserClass* InUserObject, FunctorType&& InFunctor, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddWeakLambda(InUserObject, InFunctor, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	template <typename UserClass, typename... VarTypes>
	inline TPair<TMessageId, FDelegateHandle> AddSP(TMessageId MessageId, const TSharedRef<UserClass, ESPMode::ThreadSafe>& InUserObjectRef, typename TMemFunPtrType<false, UserClass, void(TParamInvokeType, VarTypes...)>::Type InFunc, VarTypes... Vars)
	{
		FDelegateHandle result = _router.FindOrAdd(MessageId).AddSP(InUserObjectRef, InFunc, Forward<VarTypes>(Vars)...);
		return TPairInitializer<TMessageId, FDelegateHandle>(MessageId, result);
	}

	void Remove(const TPair<TMessageId, FDelegateHandle>& Handle)
	{
		if (Handle.Value.IsValid() && _router.Num() > 0)
		{
			if (MessageDelegator* delegateFunc = _router.Find(Handle.Key))
			{
				// UE5 Fix: RemoveDelegateInstance is private, use Remove instead
				delegateFunc->Remove(Handle.Value);
			}
		}
	}

	virtual void SendMessage(TMessageId MessageId, TParam inData, bool inShow = true)
	{
		if (const MessageDelegator* delegateFunc = _router.Find(MessageId))
		{
			if (inShow)
			{
				MessageHandlerLog<TMessageId>::Log(MessageId);
			}

			if (delegateFunc->IsBound())
			{
				delegateFunc->Broadcast(inData);
			}
		}
	}

	virtual void SendMessageAsync(TMessageId MessageId, TParam inData)
	{
		FRWScopeLock ScopeWriteLock(_asyncRWLock, SLT_Write);
		_asyncMessage->Emplace(MessageId, Forward<TParam>(inData));
	}

	virtual void Update() override
	{
		TArray<TGsMessage<TMessageId, TParamStoreType>>* copyQueue = nullptr;
		{
			FRWScopeLock ScopeWriteLock(_asyncRWLock, SLT_Write);
			copyQueue = _asyncMessage;
			_asyncMessage = (_asyncMessage == &_asyncMessageBufferB) ? &_asyncMessageBufferA : &_asyncMessageBufferB;
		}

		if (copyQueue)
		{
			for (TGsMessage<TMessageId, TParamStoreType>& message : *copyQueue)
			{
				SendMessage(message.GetId(), Forward<TParam>(message.ForwardParam()));
			}
			copyQueue->Reset();
		}
	}
};
