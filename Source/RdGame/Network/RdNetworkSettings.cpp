
#include "RdNetworkSettings.h"
#include "RdRemoteCharacter.h"

URdNetworkSettings::URdNetworkSettings() {
  // 기본값 설정 (C++ 클래스를 기본으로)
  RemoteCharacterClass = ARdRemoteCharacter::StaticClass();
}
