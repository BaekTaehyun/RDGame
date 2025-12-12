#pragma once

#include "CoreMinimal.h"

/**
 * IGsMessageParam
 * Base interface for all message parameters.
 */
struct IGsMessageParam
{
public:
	IGsMessageParam() = default;
	virtual ~IGsMessageParam() = default;

public:
	template <typename T = IGsMessageParam>
	T* Cast() const
	{
		static_assert(TIsDerivedFrom<T, IGsMessageParam>::IsDerived, "T must be derived from IGsMessageParam");
		return (T*)this; // Unsafe cast for performance, ensure type safety at call site or use dynamic_cast if RTTI enabled
	}
};

// Primitive Wrappers
struct FGsPrimitiveInt32 : public IGsMessageParam
{
	int32 Value = 0;
	FGsPrimitiveInt32(int32 InValue) : Value(InValue) {}
};

struct FGsPrimitiveString : public IGsMessageParam
{
	FString Value;
	FGsPrimitiveString(const FString& InValue) : Value(InValue) {}
};

struct FGsPrimitiveBool : public IGsMessageParam
{
	bool Value = false;
	FGsPrimitiveBool(bool InValue) : Value(InValue) {}
};
