#pragma once

/**
 * MACRO: GS_DECLARE_MESSAGE_ID
 * Defines the Enum class for the message IDs.
 */
#define GS_DECLARE_MESSAGE_ID(EnumName, ...) \
	enum class EnumName : uint8 \
	{ \
		__VA_ARGS__ \
	};

/**
 * MACRO: GS_DECLARE_MESSAGE_PARAM
 * Defines a struct for message parameters inheriting from IGsMessageParam.
 */
#define GS_DECLARE_MESSAGE_PARAM(StructName, ...) \
	struct StructName : public IGsMessageParam \
	{ \
		__VA_ARGS__ \
		StructName() = default; \
	};

/**
 * MACRO: GS_DEFINE_HANDLER_ALIAS
 * Defines a type alias for the handler to make code cleaner.
 */
#define GS_DEFINE_HANDLER_ALIAS(AliasName, MessageIdType, ParamType) \
	using AliasName = TGsMessageHandlerOneParam<MessageIdType, ParamType>;

/**
 * MACRO: GS_DEFINE_HANDLER_VOID_ALIAS
 * Defines a type alias for a void handler.
 */
#define GS_DEFINE_HANDLER_VOID_ALIAS(AliasName, MessageIdType) \
	using AliasName = TGsMessageHandler<MessageIdType>;
