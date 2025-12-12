#include "DungeonGeneratorTestActor.h"
#include "Kismet/GameplayStatics.h"
#include "ObjectPlacer.h"
#include "Rendering/DungeonTileRenderer.h"
#include "Algorithms/BSPGenerator.h"
#include "Algorithms/CellularAutomataGenerator.h"

ADungeonGeneratorTestActor::ADungeonGeneratorTestActor() {
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	WallMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("WallMesh"));
	WallMesh->SetupAttachment(RootComponent);
	// WallMesh->SetCanEverAffectNavigation(false); // Enable NavMesh update

	FloorMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("FloorMesh"));
	FloorMesh->SetupAttachment(RootComponent);
	// FloorMesh->SetCanEverAffectNavigation(false);

	PropMesh = CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PropMesh"));
	PropMesh->SetupAttachment(RootComponent);
	// PropMesh->SetCanEverAffectNavigation(false);
}

void ADungeonGeneratorTestActor::BeginPlay() {
	Super::BeginPlay();
}

void ADungeonGeneratorTestActor::Generate() {
	Clear();

	FDungeonGrid Grid;
	UGameInstance* GI = GetGameInstance();

	// Case 1: Runtime (Subsystem available)
	if (GI) {
		UDungeonGeneratorSubsystem* DungeonSubsystem = GI->GetSubsystem<UDungeonGeneratorSubsystem>();
		if (DungeonSubsystem) {
			Grid = DungeonSubsystem->GenerateDungeon(Seed, Width, Height, Algorithm);
		}
	}
	// Case 2: Editor (No Subsystem, instantiate directly)
#if WITH_EDITOR
	else {
		UE_LOG(LogTemp, Log, TEXT("DungeonGeneratorTestActor: Running in Editor Mode (No GameInstance)"));
		UDungeonAlgorithm* Algo = nullptr;
		switch (Algorithm) {
		case EDungeonAlgorithmType::BSP:
			Algo = NewObject<UBSPGenerator>(this);
			break;
		case EDungeonAlgorithmType::CellularAutomata:
			Algo = NewObject<UCellularAutomataGenerator>(this);
			break;
		}

		if (Algo) {
			Grid.Init(Width, Height, ETileType::Wall);
			FRandomStream RandomStream(Seed);
			Algo->Generate(Grid, RandomStream);
		}
	}
#endif

	// Validation: Ensure Meshes are assigned
	if (!WallMesh->GetStaticMesh()) {
		UE_LOG(LogTemp, Warning, TEXT("DungeonGeneratorTestActor: WallMesh is not assigned! Generation aborted."));
		return;
	}

	// Render using DungeonTileRenderer
	UDungeonTileRenderer* Renderer = NewObject<UDungeonTileRenderer>(this);
	if (Renderer) {
		Renderer->TileSize = TileSize;
		Renderer->WallPivotOffset = WallPivotOffset;
        Renderer->FloorPivotOffset = FloorPivotOffset;
        Renderer->CeilingPivotOffset = FloorPivotOffset; // Assuming ceiling follows floor
		
		Renderer->bGenerateFloor = true;
		Renderer->WallMeshTable.Add(0, WallMesh->GetStaticMesh());
		Renderer->FloorMesh = FloorMesh ? FloorMesh->GetStaticMesh() : nullptr;
		Renderer->CeilingMesh = nullptr; // Explicitly no ceiling mesh for now if not assigned

		Renderer->GenerateBSPTiles(Grid, WallMesh, nullptr, FloorMesh);
	}

	// Place Props using Poisson Disk Sampling
	if (PropMesh && PropMesh->GetStaticMesh()) {
		FRandomStream RandomStream(Seed);
		TArray<FVector2D> PropPoints = UObjectPlacer::GeneratePoissonPoints(Grid, 5.0f, 100, RandomStream);
		for (const FVector2D& P : PropPoints) {
			FTransform Transform;
			// Use FloorPivotOffset for props as they sit on the floor
			Transform.SetLocation(FVector(P.X * TileSize, P.Y * TileSize, 50.0f) + FloorPivotOffset);
			PropMesh->AddInstance(Transform);
		}
	}

    // Trigger Navigation Rebuild with explicit Bounds + Offset
	UDungeonNavigationBuilder* NavBuilder = NewObject<UDungeonNavigationBuilder>(this);
	if (NavBuilder) {
		NavBuilder->TileSize = TileSize;
		NavBuilder->WallHeight = 300.0f; 
		
		// Use FloorPivotOffset as the primary nav offset
		FBox NavBounds = NavBuilder->CalculateNavBounds(Grid, FloorPivotOffset);
		NavBuilder->TriggerNavMeshRebuild(GetWorld(), NavBounds);
	}
}

void ADungeonGeneratorTestActor::Clear() {
	WallMesh->ClearInstances();
	FloorMesh->ClearInstances();
	PropMesh->ClearInstances();
}
