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

	// Spawn Graphs based on Theme properties and CompatibilityMode
	// Using properties defined in DungeonThemeAsset.h

	if (Theme->CompatibilityMode == EDungeonAlgorithmType::BSP)
	{
		// --- BSP Mode: Structural generation ---
		
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

		// 2. Room-specific
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

		// 3. Fallback Floor
		if (Theme->FloorPCGGraph)
		{
			SpawnPCGGraph(OwnerActor, Theme->FloorPCGGraph, TEXT("PCG_Floor"), Seed);
		}
	}
	else if (Theme->CompatibilityMode == EDungeonAlgorithmType::CellularAutomata)
	{
		// --- Cellular Automata Mode: Organic/Nature generation ---
		// Note: Landscape is used as floor, no Floor meshes spawned.
		UE_LOG(LogTemp, Warning, TEXT("UDungeonPCGRenderer: CA Mode detected. NaturePCGGraph=%s, PathPCGGraph=%s"),
			Theme->NaturePCGGraph ? *Theme->NaturePCGGraph->GetName() : TEXT("NULL"),
			Theme->PathPCGGraph ? *Theme->PathPCGGraph->GetName() : TEXT("NULL"));

		// Nature assets on non-walkable areas (Wall tiles = trees, rocks)
		if (Theme->NaturePCGGraph)
		{
			UE_LOG(LogTemp, Warning, TEXT("UDungeonPCGRenderer: Spawning NaturePCGGraph..."));
			SpawnPCGGraph(OwnerActor, Theme->NaturePCGGraph, TEXT("PCG_Nature"), Seed);
		}

		// Optional: Path decoration on walkable areas (Floor tiles = grass, flowers)
		if (Theme->PathPCGGraph)
		{
			SpawnPCGGraph(OwnerActor, Theme->PathPCGGraph, TEXT("PCG_Path"), Seed);
		}
	}

	// Log count by querying the actor
	UE_LOG(LogTemp, Log, TEXT("UDungeonPCGRenderer: Generated PCG components with Seed: %d (Mode: %s)"), 
		Seed, Theme->CompatibilityMode == EDungeonAlgorithmType::BSP ? TEXT("BSP") : TEXT("CellularAutomata"));
}

// Include GameplayStatics for Tag search
#include "Kismet/GameplayStatics.h" 
#include "Engine/TargetPoint.h"
#include "EngineUtils.h"
#include "PCGVolume.h"
#include "Components/BrushComponent.h"

#if WITH_EDITOR
#include "Editor.h"
#endif

void UDungeonPCGRenderer::Cleanup(AActor* Owner)
{
	UE_LOG(LogTemp, Warning, TEXT("UDungeonPCGRenderer::Cleanup(Owner) called"));
	
	if (!Owner) return;
	
	UWorld* World = Owner->GetWorld();
	if (!World) return;

	// 1. Cleanup Anchor Actors / Volumes (created by SpawnPCGGraph)
	// Managed via Tags - PCG components are on these actors, not on Owner
	TArray<AActor*> Anchors;
	UGameplayStatics::GetAllActorsWithTag(World, FName("DungeonPCGAnchor"), Anchors);
	
	UE_LOG(LogTemp, Log, TEXT("UDungeonPCGRenderer::Cleanup() - Found %d Anchor Actors"), Anchors.Num());
	for (AActor* A : Anchors)
	{
		if (A && IsValid(A)) 
		{
			// Clean up PCG component on the anchor before destroying
			if (UPCGComponent* AnchorPCG = A->FindComponentByClass<UPCGComponent>())
			{
				AnchorPCG->CleanupLocalImmediate(true, true);
			}

			// Rename to free up the name immediately (PendingKill doesn't release name instantly)
			FString CompName = A->GetName();
			A->Rename(*FString::Printf(TEXT("TRASH_%s"), *CompName), nullptr, REN_DontCreateRedirectors);
			A->Destroy();
		}
	}
	TArray<UInstancedStaticMeshComponent*> ISMComponents;
	Owner->GetComponents<UInstancedStaticMeshComponent>(ISMComponents);
	for (UInstancedStaticMeshComponent* ISM : ISMComponents)
	{
		if (ISM) ISM->DestroyComponent();
	}
	
	TArray<UStaticMeshComponent*> SMComponents;
	Owner->GetComponents<UStaticMeshComponent>(SMComponents);
	for (UStaticMeshComponent* SM : SMComponents)
	{
		if (SM) SM->DestroyComponent();
	}
	
#if WITH_EDITOR
	// 4. Destroy PCGWorldActor
	if (UPCGSubsystem* PCGSubsystem = UPCGSubsystem::GetInstance(World))
	{
		PCGSubsystem->DestroyCurrentPCGWorldActor();
	}
#endif
}


void UDungeonPCGRenderer::SpawnPCGGraph(AActor* Owner, UPCGGraph* Graph, const FName& Name, int32 Seed)
{
	if (!Graph || !Owner) return;

	UWorld* World = Owner->GetWorld();
	if (!World) return;

#if WITH_EDITOR
	// Use ActorFactory for proper brush geometry initialization (same as editor drag & drop)
	UActorFactory* PCGVolumeFactory = GEditor->FindActorFactoryForActorClass(APCGVolume::StaticClass());
	if (!PCGVolumeFactory)
	{
		UE_LOG(LogTemp, Error, TEXT("UDungeonPCGRenderer: Failed to find PCGVolumeFactory!"));
		return;
	}

	// Find Landscape to get actual bounds
	FVector LandscapeOrigin = FVector::ZeroVector;
	FVector LandscapeExtent = FVector(10000, 10000, 5000); // Default fallback
	
	TArray<AActor*> Landscapes;
	UGameplayStatics::GetAllActorsWithTag(World, FName("DungeonGeneratedLandscape"), Landscapes);
	if (Landscapes.Num() > 0 && Landscapes[0])
	{
		Landscapes[0]->GetActorBounds(false, LandscapeOrigin, LandscapeExtent);
		UE_LOG(LogTemp, Log, TEXT("Found Landscape: Origin=%s, Extent=%s"), *LandscapeOrigin.ToString(), *LandscapeExtent.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("No DungeonGeneratedLandscape found, using default bounds"));
	}

	// Spawn at Landscape center
	FVector SpawnLoc = LandscapeOrigin;

	FTransform ActorTransform(FRotator::ZeroRotator, SpawnLoc);
	// NOTE: Don't set scale in transform - ActorFactory may override it
	// We'll set it after spawn
	
	APCGVolume* PCGVol = Cast<APCGVolume>(
		GEditor->UseActorFactory(PCGVolumeFactory, FAssetData(APCGVolume::StaticClass()), &ActorTransform)
	);
	
	if (PCGVol)
	{
		PCGVol->SetActorLabel(Name.ToString());
		PCGVol->Tags.Add(FName("DungeonPCGAnchor"));
		
		// Get actual brush size after spawn (no hardcoding!)
		FVector CurrentOrigin, CurrentExtent;
		PCGVol->GetActorBounds(false, CurrentOrigin, CurrentExtent);
		
		// Calculate scale multiplier to match Landscape
		// Avoid division by zero
		float ScaleX = (CurrentExtent.X > 0) ? (LandscapeExtent.X / CurrentExtent.X) : 1.0f;
		float ScaleY = (CurrentExtent.Y > 0) ? (LandscapeExtent.Y / CurrentExtent.Y) : 1.0f;
		float ScaleZ = 100.0f; // Fixed height (10km coverage)
		
		UE_LOG(LogTemp, Log, TEXT("PCGVolume: CurrentExtent=%s, TargetExtent=%s, Scale=(%f, %f, %f)"), 
			*CurrentExtent.ToString(), *LandscapeExtent.ToString(), ScaleX, ScaleY, ScaleZ);
		
		PCGVol->SetActorScale3D(FVector(ScaleX, ScaleY, ScaleZ));
		
		// Force bounds update so PCG recognizes the new scale immediately
		PCGVol->UpdateComponentTransforms();
		if (UBrushComponent* BrushComp = PCGVol->GetBrushComponent())
		{
			BrushComp->UpdateBounds();
			BrushComp->MarkRenderStateDirty();
		}

		// Configure PCG Component
		if (UPCGComponent* PCGComp = PCGVol->GetComponentByClass<UPCGComponent>())
		{
			PCGComp->SetGraph(Graph);
			PCGComp->Seed = Seed; 
			PCGComp->GenerationTrigger = EPCGComponentGenerationTrigger::GenerateOnDemand;
			PCGComp->bActivated = true;
			
			// Mark as dirty to force recognition of new bounds (same as moving in editor)
			PCGComp->DirtyGenerated(EPCGComponentDirtyFlag::Actor);
			
			// Trigger Generation via Subsystem (same as editor)
			if (UPCGSubsystem* Subsystem = PCGComp->GetSubsystem())
			{
				TWeakObjectPtr<UPCGComponent> ComponentPtr(PCGComp);
				Subsystem->ScheduleGeneric([ComponentPtr]()
				{
					if (UPCGComponent* Component = ComponentPtr.Get())
					{
						if (IsValid(Component))
						{
							Component->Generate();
						}
					}
					return true;
				}, PCGComp, /*TaskDependencies=*/{});
			}
		}
	}
#else
	UE_LOG(LogTemp, Warning, TEXT("UDungeonPCGRenderer: SpawnPCGGraph only works in Editor!"));
#endif
}
