#pragma once

#include "CoreInterfaces.h"
#include "Logging/LogMacros.h"
#include "Math/RandomStream.h"


DECLARE_LOG_CATEGORY_EXTERN(LogDungeonCore, Log, All);

class FUnrealRandomAdapter : public DungeonCore::IRandom {
public:
  FRandomStream RandomStream;

  FUnrealRandomAdapter(int32 Seed) : RandomStream(Seed) {}
  FUnrealRandomAdapter(const FRandomStream &InStream)
      : RandomStream(InStream) {}

  virtual void Init(int32_t Seed) override { RandomStream.Initialize(Seed); }

  virtual int32_t GetInitialSeed() const override {
    return RandomStream.GetInitialSeed();
  }

  virtual float GetFraction() override { return RandomStream.GetFraction(); }

  virtual int32_t RandRange(int32_t Min, int32_t Max) override {
    return RandomStream.RandRange(Min, Max);
  }
};

class FUnrealLoggerAdapter : public DungeonCore::ILogger {
public:
  virtual void LogInfo(const char *Format, ...) override {
    va_list Args;
    va_start(Args, Format);
    char Buffer[1024];
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);
    va_end(Args);
    UE_LOG(LogDungeonCore, Log, TEXT("%s"), ANSI_TO_TCHAR(Buffer));
  }

  virtual void LogWarning(const char *Format, ...) override {
    va_list Args;
    va_start(Args, Format);
    char Buffer[1024];
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);
    va_end(Args);
    UE_LOG(LogDungeonCore, Warning, TEXT("%s"), ANSI_TO_TCHAR(Buffer));
  }

  virtual void LogError(const char *Format, ...) override {
    va_list Args;
    va_start(Args, Format);
    char Buffer[1024];
    vsnprintf(Buffer, sizeof(Buffer), Format, Args);
    va_end(Args);
    UE_LOG(LogDungeonCore, Error, TEXT("%s"), ANSI_TO_TCHAR(Buffer));
  }
};
