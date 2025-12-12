#pragma once

#include <cstdint>
#include <string>


namespace DungeonCore {

// 난수 생성기 인터페이스 (플랫폼 독립적)
class IRandom {
public:
  virtual ~IRandom() = default;

  virtual void Init(int32_t Seed) = 0;
  virtual int32_t GetInitialSeed() const = 0;
  virtual float GetFraction() = 0; // 0.0 ~ 1.0 사이의 실수 반환
  virtual int32_t RandRange(int32_t Min,
                            int32_t Max) = 0; // Min ~ Max 사이의 정수 반환
};

// 로거 인터페이스 (플랫폼 독립적)
class ILogger {
public:
  virtual ~ILogger() = default;

  virtual void LogInfo(const char *Format, ...) = 0;
  virtual void LogWarning(const char *Format, ...) = 0;
  virtual void LogError(const char *Format, ...) = 0;
};

} // namespace DungeonCore
