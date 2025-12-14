#include "Data/DungeonPresetData.h"

TArray<FModuleData>
UPresetModuleDatabase::GetModulesByType(EDungeonPresetModuleType InType) const {
  TArray<FModuleData> Result;
  for (const FModuleData &Module : Modules) {
    if (Module.Type == InType) {
      Result.Add(Module);
    }
  }
  return Result;
}
