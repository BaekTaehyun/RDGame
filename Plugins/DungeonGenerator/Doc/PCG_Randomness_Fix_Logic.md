# PCG Randomness & Refresh Fix - Technical Logic

## Problem
Changing the Dungeon Seed did not result in a new PCG layout, even though the underlying Grid data was updating correctly.
The issue was identified as a combination of:
1. **PCG Caching**: The `Dungeon Data Reader` node was being cached by the PCG subsystem, causing it to return old data even when the Seed changed.
2. **Component Lifecycle**: Dynamically created `UPCGComponent` instances were being Garbage Collected or destroyed too quickly (or not persisting in Editor) to execute their logic fully.

## Solution Logic

### 1. Force PCG Execution (Disable Caching)
Modified `UPCGDungeonDataReaderSettings` to explicitly disable caching. This forces the node to execute its `ExecuteInternal` logic on every generation request.

**File:** `Source/DungeonGenerator/Public/PCG/PCGDungeonDataReader.h`
```cpp
public:
    // Disable caching to ensure we always read the latest Grid data from the Actor
    virtual bool IsCacheable() const override { return false; }
```

### 2. PCG Component Persistence & Tracking
Updated `UDungeonPCGRenderer` to keep a strong reference to created PCG components and ensure they persist in the Editor environment.

**File:** `Source/DungeonGenerator/Public/Rendering/DungeonPCGRenderer.h`
```cpp
UPROPERTY()
TArray<TObjectPtr<class UPCGComponent>> SpawnedPCGComponents;
```

**File:** `Source/DungeonGenerator/Private/Rendering/DungeonPCGRenderer.cpp`
```cpp
// Creation
PCGComp->CreationMethod = EComponentCreationMethod::Instance; // Persist like a normal component
SpawnedPCGComponents.Add(PCGComp); // Add to tracking array

// Cleanup
for (TObjectPtr<UPCGComponent> PCG : SpawnedPCGComponents)
{
    if (PCG && IsValid(PCG))
    {
        PCG->CleanupLocalImmediate(true, true);
        PCG->DestroyComponent();
    }
}
SpawnedPCGComponents.Empty();
```

### 3. Editor UX Improvement
Hooked into `PostEditChangeProperty` to automatically trigger regeneration when the Seed property is modified in the Details panel.

**File:** `Source/DungeonGenerator/Private/DungeonWorldBuilder.cpp`
```cpp
if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeonWorldBuilder, SeedOverride) || ...)
{
    Generate();
}
```
