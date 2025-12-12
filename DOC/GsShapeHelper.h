// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"

/**
 * FGsShapeHelper
 * 
 * Stateless utility class for geometric collision detection.
 * Designed to separate math logic from game objects for better reusability and testing.
 */
class T1PROJECT_API FGsShapeHelper
{
public:
	/**
	 * [Circle Collision]
	 * Checks if a point (or circle) is inside a Circle.
	 * 
	 *        ( r )
	 *      /       \
	 *    (   Center  )  <--- Distance ---> ( Target )
	 *      \       /
	 * 
	 * Formula: DistSq(Center, Point) <= (Radius + TargetRadius)^2
	 */
	static bool IsPointInCircle(const FVector2D& Point, const FVector2D& Center, float Radius, float TargetRadius = 0.f);

	/**
	 * [Fan/Sector Collision]
	 * Checks if a point (or circle) is inside a Fan shape.
	 * 
	 *       \       /   ^
	 *        \     /    | Radius
    *         \   /     v
	 *          \ /  Angle
	 *        (Center)
	 * 
	 * Logic:
	 * 1. Distance check: Dist <= Radius + TargetRadius
	 * 2. Angle check: Angle(Direction, DirToPoint) <= HalfAngle
	 * 3. Volume check (If TargetRadius > 0): 
	 *    If point is outside Angle but close to edges, checks distance to Edge Segments.
	 */
	static bool IsPointInFan(const FVector2D& Point, const FVector2D& Center, const FVector2D& Direction, 
		float Radius, float AngleDeg, float TargetRadius = 0.f);

	/**
	 * [Rect/Box Collision]
	 * Checks if a point (or circle) is inside a Rotated Rectangle.
	 * 
	 *    Edges[0] (Top-Left) _______________________ Edges[1] (Top-Right)
	 *                       |                       |
	 *                       |        Center         |
	 *                       |           x           |
	 *    Edges[3] (Bot-Left)|_______________________| Edges[2] (Bot-Right)
	 * 
	 * Logic:
	 * 1. Transform Point to Rect's Local Space (using Center & Direction).
	 * 2. AABB Check in Local Space:
	 *    (Abs(LocalX) <= HalfHeight + TargetRadius) && (Abs(LocalY) <= HalfWidth + TargetRadius)
	 *    *Note: Assumes Direction is Forward (X-Axis).
	 */
	static bool IsPointInRect(const FVector2D& Point, const FVector2D& Center, const FVector2D& Direction, 
		const FVector2D& Extents, float TargetRadius = 0.f);
};
