// Fill out your copyright notice in the Description page of Project Settings.

#include "GsShapeHelper.h"
#include "Math/UnrealMathUtility.h"

bool FGsShapeHelper::IsPointInCircle(const FVector2D& Point, const FVector2D& Center, float Radius, float TargetRadius)
{
	float distSq = FVector2D::DistSquared(Point, Center);
	float combineRadius = Radius + TargetRadius;
	
	return distSq <= (combineRadius * combineRadius);
}

bool FGsShapeHelper::IsPointInFan(const FVector2D& Point, const FVector2D& Center, const FVector2D& Direction,
	float Radius, float AngleDeg, float TargetRadius)
{
	// 1. Distance Check (Broad Phase)
	float distSq = FVector2D::DistSquared(Point, Center);
	float maxRadius = Radius + TargetRadius;
	if (distSq > maxRadius * maxRadius)
	{
		return false;
	}

	FVector2D dirToPoint = Point - Center;
	if (dirToPoint.IsNearlyZero())
	{
		return true; // Exactly on center
	}
	dirToPoint.Normalize();

	// 2. Angle Check
	// Dot Product returns Cos(Angle)
	// If Angle <= HalfAngle, then Cos(Angle) >= Cos(HalfAngle)
	// (Note: Cosine decreases as Angle increases from 0 to 180)
	
	float halfAngleRad = FMath::DegreesToRadians(AngleDeg * 0.5f);
	float thresholdCos = FMath::Cos(halfAngleRad);
	float dot = FVector2D::DotProduct(Direction, dirToPoint);

	if (dot >= thresholdCos)
	{
		return true; // Inside the pure angular sector
	}

	// 3. Volume Check (Target Radius)
	// If strict check failed, but the target has size, it might be touching the edges.
	if (TargetRadius <= 0.f)
	{
		return false;
	}

	// Robust Edge Check:
	// We check distance to the two edge SEGMENTS (not infinite lines).
	// Edge Segment = Center to (Center + Radius * EdgeDir)
	
	// Left Edge (Rotated -HalfAngle)
	float sinHalf, cosHalf;
	FMath::SinCos(&sinHalf, &cosHalf, halfAngleRad);
	
	// Rotate Direction by -HalfAngle
	// x' = x*cos - y*sin
	// y' = x*sin + y*cos
	FVector2D leftEdgeDir;
	leftEdgeDir.X = Direction.X * cosHalf - Direction.Y * -sinHalf;
	leftEdgeDir.Y = Direction.X * -sinHalf + Direction.Y * cosHalf;

	// Right Edge (Rotated +HalfAngle)
	FVector2D rightEdgeDir;
	rightEdgeDir.X = Direction.X * cosHalf - Direction.Y * sinHalf;
	rightEdgeDir.Y = Direction.X * sinHalf + Direction.Y * cosHalf;

	// Check distance to Left Edge Segment
	FVector2D leftEdgeEnd = Center + leftEdgeDir * Radius;
	float distToLeft = FMath::PointDistToSegment(FVector(Point.X, Point.Y, 0), FVector(Center.X, Center.Y, 0), FVector(leftEdgeEnd.X, leftEdgeEnd.Y, 0));
	
	if (distToLeft <= TargetRadius)
	{
		return true;
	}

	// Check distance to Right Edge Segment
	FVector2D rightEdgeEnd = Center + rightEdgeDir * Radius;
	float distToRight = FMath::PointDistToSegment(FVector(Point.X, Point.Y, 0), FVector(Center.X, Center.Y, 0), FVector(rightEdgeEnd.X, rightEdgeEnd.Y, 0));

	if (distToRight <= TargetRadius)
	{
		return true;
	}

	return false;
}

bool FGsShapeHelper::IsPointInRect(const FVector2D& Point, const FVector2D& Center, const FVector2D& Direction,
	const FVector2D& Extents, float TargetRadius)
{
	// 1. Local Space Conversion
	FVector2D dirDiff = Point - Center;
	
	// Unrotate using direction
	// If Direction is (1, 0) [Forward], then X is forward, Y is right.
	// We project dirDiff onto Direction (X) and Right (Y).
	
	// Project onto Forward Axis (X)
	float localX = FVector2D::DotProduct(dirDiff, Direction);
	
	// Right Vector is (-y, x) for 2D rotation 90 deg clockwise? Or (y, -x)?
	// Unreal Coords: X=Forward, Y=Right, Z=Up.
	// For 2D vector (X, Y), Right (90 deg) is (Y, -X) if Y is right?
	// Let's use standard rotation matrix inverse.
	// [ Cos  Sin ]
	// [-Sin  Cos ]
	// LocalX = X*Cos + Y*Sin = Dot(P, Dir) Correct.
	// LocalY = X*-Sin + Y*Cos = Dot(P, RightVector)
	
	// Assuming Direction is normalized
	FVector2D rightDir(Direction.Y, -Direction.X); // 90 deg right
	float localY = FVector2D::DotProduct(dirDiff, rightDir);

	// 2. AABB Check
	// Extents usually Means Box Extent (Half Size)
	// Extents.X = Half Forward Length
	// Extents.Y = Half Width
	
	float allowedX = Extents.X + TargetRadius;
	float allowedY = Extents.Y + TargetRadius;

	if (FMath::Abs(localX) <= allowedX &&
		FMath::Abs(localY) <= allowedY)
	{
		// Corner Case Check (Rounded Corners)
		// If both X and Y are outside the pure box but inside the expanded box,
		// we are in the corner zone. Check distance to corner.
		
		bool bOutsideX = FMath::Abs(localX) > Extents.X;
		bool bOutsideY = FMath::Abs(localY) > Extents.Y;
		
		if (bOutsideX && bOutsideY)
		{
			float dx = FMath::Abs(localX) - Extents.X;
			float dy = FMath::Abs(localY) - Extents.Y;
			return (dx*dx + dy*dy) <= (TargetRadius * TargetRadius);
		}
		
		return true;
	}

	return false;
}
