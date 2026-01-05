# PCG Graph Analysis: PCG_Nature_Wood

## Overview
**Asset Path**: `/Game/LevelPrototyping/PCG_Nature_Wood`  
**Analysis Date**: 2026-01-05  
**Purpose**: Spawns nature assets (trees, rocks, debris) based on a tiered attribute system (likely Distance or Density).

## Logic Structure: Cascading Filter (계단식 조건 분기)

The graph utilizes a "Cascade" pattern where points are filtered sequentially. If a point fails a condition, it falls through to the next check. This ensures mutual exclusivity between tiers.

### 1. Data Processing Phase
*   **Input**: `DungeonDataReader` imports marker points from the level/dungeon system.
*   **Grid**: `CreatePointsGrid` & `CopyPoints` populate the area.
*   **Modification**:
    *   `Distance`: Calculates distance fields (probably from walls or centers).
    *   `TransformPoints`: Applies rotation/scale randomization.
    *   `BoundsModifier` & `SelfPruning`: Cleans up overlapping or out-of-bounds points.

### 2. Spawning Phase (The Cascade)

Points flow through a chain of **Attribute Filters**.

*   **Tier 1 (Top Priority)**
    *   **Node**: `AttributeFilter_1`
    *   **Condition**: `GREATER` than Threshold A.
    *   **Result**: Spawns **Mesh Group 0** (4 Variants).
    *   **Else**: Passes to Tier 2.

*   **Tier 2**
    *   **Node**: `AttributeFilter_2`
    *   **Condition**: `GREATER` than Threshold B.
    *   **Result**: Spawns **Mesh Group 1** (5 Variants).
    *   **Else**: Passes to Tier 3.

*   **Tier 3**
    *   **Node**: `AttributeFilter_3`
    *   **Condition**: `GREATER` than Threshold C.
    *   **Result**: Spawns **Mesh Group 2** (5 Variants).
    *   **Else**: Passes to Tier 4.

*   **Tier 4 (Background/Remaining)**
    *   **Node**: `AttributeFilter_4`
    *   **Condition**: `LESSER_OR_EQUAL` (Catch-all for remaining low values).
    *   **Result**: Spawns **Mesh Group 3** (3 Variants).

## Summary
This setup creates a rich, layered environment where different types of assets appear at different "intensities" (e.g., big trees in dense areas, small bushes in sparse areas) without overlapping logic.
