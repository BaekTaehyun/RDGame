// Copyright 2024. bak1210(백태현) All Rights Reserved.
// 
// 파일 목적:
// 메시지 전달을 위한 기본 템플릿 객체(TGsMessage)를 정의합니다.
// Enum ID와 파라미터를 캡슐화하여 전달하는 역할을 합니다.

#pragma once

#include "CoreMinimal.h"

//------------------------------------------------------------------------------
// 메시지에 파라미터를 전달하기 위한 기본 객체 클래스
// TEnumId : Enum 타입 메시지 ID
// TParam  : 파라미터 타입
//------------------------------------------------------------------------------
template <typename TEnumId, typename TParam>
class TGsMessage
{
	static_assert(TIsEnum<TEnumId>::Value, "TEnumId is not enumerator type");

private:
	TEnumId _id;
	TParam	_param;

public:
	explicit TGsMessage(TEnumId id, TParam param) : _id(id), _param(Forward<TParam>(param))
	{
	}
	
	explicit TGsMessage(const TGsMessage<TEnumId, TParam>& other) : _id(other.GetId()), _param(other.GetParam())
	{
	}

public:
	virtual ~TGsMessage() = default;

public:
	TEnumId GetId() const { return _id; }

	TParam& GetParam() { return _param; }
	const TParam& GetParam() const { return _param; }

	TParam ForwardParam() { return Forward<TParam>(_param); }
};
