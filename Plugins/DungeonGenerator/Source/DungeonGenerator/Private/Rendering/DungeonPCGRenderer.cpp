#include "Rendering/DungeonPCGRenderer.h"
#include "Data/DungeonThemeAsset.h"
#include "PCGComponent.h"
#include "PCGGraph.h"
#include "Subsystems/PCGSubsystem.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"

UDungeonPCGRenderer::UDungeonPCGRenderer()
{
}

// Note: GeneratePCG signature changed
void UDungeonPCGRenderer::GeneratePCG(const FDungeonGrid& Grid, AActor* OwnerActor, const UDungeonThemeAsset* Theme, int32 Seed)
{
	if (!OwnerActor)
	{
		UE_LOG(LogTemp, Error, TEXT("UDungeonPCGRenderer: OwnerActor is null"));
		return;
	}

	if (!Theme)
	{
		UE_LOG(LogTemp, Warning, TEXT("UDungeonPCGRenderer: No Theme provided. Skipping PCG generation."));
		return;
	}

	// Store Grid for bounds calculation
	CachedGrid = Grid;
	
	// Store Owner for cleanup after level load
	CachedOwner = OwnerActor;

	// Clean up existing
	Cleanup(OwnerActor);

	// Spawn Graphs based on Theme properties
	// Using properties defined in DungeonThemeAsset.h

	// 1. Walls
	if (Theme->WallPCGGraph)
	{
		SpawnPCGGraph(OwnerActor, Theme->WallPCGGraph, TEXT("PCG_Wall"), Seed);
	}

	// 1.5. Corner Walls (fills gaps at wall corners)
	if (Theme->CornerWallPCGGraph)
	{
		SpawnPCGGraph(OwnerActor, Theme->CornerWallPCGGraph, TEXT("PCG_CornerWall"), Seed);
	}

	// 1.6. Through Walls (walkable on both sides)
	if (Theme->ThroughWallPCGGraph)
	{
		SpawnPCGGraph(OwnerActor, Theme->ThroughWallPCGGraph, TEXT("PCG_ThroughWall"), Seed);
	}

	// 2. Floors (Corridor vs Room differentiation is handled by the PCG Graph logic reading from Point Data)
	// Usually we might want separate Graphs spawned, or one Graph handling multiple filters.
	// Based on Theme, we have distinct slots.

	if (Theme->CompatibilityMode == EDungeonAlgorithmType::BSP)
	{
		if (Theme->RoomPCGGraph)
		{
			SpawnPCGGraph(OwnerActor, Theme->RoomPCGGraph, TEXT("PCG_Room"), Seed);
		}

		if (Theme->CorridorPCGGraph)
		{
			SpawnPCGGraph(OwnerActor, Theme->CorridorPCGGraph, TEXT("PCG_Corridor"), Seed);
		}
		
		if (Theme->DoorPCGGraph)
		{
			SpawnPCGGraph(OwnerActor, Theme->DoorPCGGraph, TEXT("PCG_Door"), Seed);
		}
	}

	// 3. Fallback Floor (if used)
	if (Theme->FloorPCGGraph)
	{
		SpawnPCGGraph(OwnerActor, Theme->FloorPCGGraph, TEXT("PCG_Floor"), Seed);
	}

	// Log count by querying the actor
	UE_LOG(LogTemp, Log, TEXT("UDungeonPCGRenderer: Generated PCG components with Seed: %d"), Seed);
}

void UDungeonPCGRenderer::Cleanup(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("UDungeonPCGRenderer::Cleanup(Owner) called"));
	
	if (!Owner)
	{
		UE_LOG(LogTemp, Error, TEXT("UDungeonPCGRenderer::Cleanup() - Owner is null!"));
		return;
	}
	
	// 1. Cleanup and destroy PCG components from tracked list
	UE_LOG(LogTemp, Log, TEXT("UDungeonPCGRenderer::Cleanup() - Found %d Tracked PCG components"), SpawnedPCGComponents.Num());
	
	for (TObjectPtr<UPCGComponent> PCG : SpawnedPCGComponents)
	{
		if (PCG && IsValid(PCG))
		{
			PCG->CleanupLocalImmediate(true, true);
			PCG->DestroyComponent();
		}
	}
	SpawnedPCGComponents.Empty();
	
	// 2. Destroy all ISM components created by PCG (they are children of owner)
	TArray<UInstancedStaticMeshComponent*> ISMComponents;
	Owner->GetComponents<UInstancedStaticMeshComponent>(ISMComponents);
	
	UE_LOG(LogTemp, Log, TEXT("UDungeonPCGRenderer::Cleanup() - Found %d ISM components"), ISMComponents.Num());
	
	for (UInstancedStaticMeshComponent* ISM : ISMComponents)
	{
		if (ISM && IsValid(ISM))
		{
			ISM->DestroyComponent();
		}
	}
	
	// 3. Destroy any StaticMeshComponents as well (some PCG nodes create these)
	TArray<UStaticMeshComponent*> SMComponents;
	Owner->GetComponents<UStaticMeshComponent>(SMComponents);
	
	for (UStaticMeshComponent* SM : SMComponents)
	{
		if (SM && IsValid(SM))
		{
			SM->DestroyComponent();
		}
	}
	
#if WITH_EDITOR
	// 4. Destroy PCGWorldActor to reset PCG caching
	if (UWorld* World = Owner->GetWorld())
	{
		if (UPCGSubsystem* PCGSubsystem = UPCGSubsystem::GetInstance(World))
		{
			PCGSubsystem->DestroyCurrentPCGWorldActor();
			UE_LOG(LogTemp, Log, TEXT("UDungeonPCGRenderer::Cleanup() - Destroyed PCGWorldActor"));
		}
	}
#endif
	
	UE_LOG(LogTemp, Warning, TEXT("UDungeonPCGRenderer::Cleanup() complete"));
}


void UDungeonPCGRenderer::SpawnPCGGraph(AActor* Owner, UPCGGraph* Graph, const FName& Name, int32 Seed)
{
	if (!Graph || !Owner) return;

	UPCGComponent* PCGComp = NewObject<UPCGComponent>(Owner, Name);
	if (PCGComp)
	{
		PCGComp->RegisterComponent();
		Owner->AddInstanceComponent(PCGComp);
		PCGComp->CreationMethod = EComponentCreationMethod::Instance; // Changed to Instance to persist like normal components
		
		SpawnedPCGComponents.Add(PCGComp); // Track it!

		PCGComp->SetGraph(Graph);
		PCGComp->Seed = Seed; 
		
		// Configure for immediate generation
		PCGComp->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
		
		// Just ensure bActivated is true so it processes
		PCGComp->bActivated = true;
		
		// GenerateLocal(true) forces fresh generation without using cached data
		PCGComp->GenerateLocal(true);
	}
}
