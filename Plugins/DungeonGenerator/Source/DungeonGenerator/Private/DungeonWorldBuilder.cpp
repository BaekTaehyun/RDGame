#include "DungeonWorldBuilder.h"
#include "Components/DungeonRendererComponent.h"
#include "Components/BoxComponent.h"
#include "Algorithms/BSPGenerator.h"
#include "Algorithms/CellularAutomataGenerator.h"
#include "EngineUtils.h"
#include "Landscape.h"

// Define the static delegate
FOnRequestDungeonLandscape ADungeonWorldBuilder::OnRequestLandscape;

// Including Theme Asset for soft pointer loading
#include "Data/DungeonThemeAsset.h"

ADungeonWorldBuilder::ADungeonWorldBuilder()
{
	PrimaryActorTick.bCanEverTick = false;

	// Create Box Component as Root (provides bounds for PCG)
	BoundsBox = CreateDefaultSubobject<UBoxComponent>(TEXT("BoundsBox"));
	RootComponent = BoundsBox;
	BoundsBox->SetBoxExtent(FVector(2500.0f, 2500.0f, 200.0f)); // Default 50x50 grid * 100 tile size
	BoundsBox->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	BoundsBox->SetVisibility(false);

	// Create Core Component (Logic Only)
	DungeonRenderer = CreateDefaultSubobject<UDungeonRendererComponent>(TEXT("DungeonRenderer"));

	// Init Default Config
	GeneratorConfig.Seed = FMath::Rand();
	GeneratorConfig.Algorithm = EDungeonAlgorithmType::BSP;
	GeneratorConfig.RenderMode = EDungeonRenderMode::PCG; // Default to PCG for WorldBuilder
}

void ADungeonWorldBuilder::BeginPlay()
{
	Super::BeginPlay();

	// Load Config from Table if set
	if (ConfigTable.DataTable && !ConfigTable.RowName.IsNone())
	{
		static const FString ContextString(TEXT("DungeonWorldBuilder_BeginPlay"));
		const FDungeonGenConfig* Row = ConfigTable.DataTable->FindRow<FDungeonGenConfig>(ConfigTable.RowName, ContextString);
		if (Row)
		{
			GeneratorConfig = *Row;
			if(Row->Theme.LoadSynchronous())
			{
				DungeonTheme = Row->Theme.Get();
			}
		}
	}
}

#if WITH_EDITOR
void ADungeonWorldBuilder::PostEditChangeProperty(FPropertyChangedEvent& PropertyChangedEvent)
{
	Super::PostEditChangeProperty(PropertyChangedEvent);

	// Auto-generate if SeedOverride related properties change
	FName PropertyName = (PropertyChangedEvent.Property != nullptr) ? PropertyChangedEvent.Property->GetFName() : NAME_None;
	if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeonWorldBuilder, SeedOverride) ||
		PropertyName == GET_MEMBER_NAME_CHECKED(ADungeonWorldBuilder, bUseSeedOverride))
	{
		Generate();
	}

	if (PropertyName == GET_MEMBER_NAME_CHECKED(ADungeonWorldBuilder, ConfigTable))
	{
		if (ConfigTable.DataTable && !ConfigTable.RowName.IsNone())
		{
			static const FString ContextString(TEXT("DungeonWorldBuilder_PostEditChange"));
			const FDungeonGenConfig* Row = ConfigTable.DataTable->FindRow<FDungeonGenConfig>(ConfigTable.RowName, ContextString);
			if (Row)
			{
				GeneratorConfig = *Row;
				if(Row->Theme.LoadSynchronous())
				{
					DungeonTheme = Row->Theme.Get();
				}
			}
		}
	}
}
#endif

void ADungeonWorldBuilder::Generate()
{
	UE_LOG(LogTemp, Warning, TEXT("[Input Check] bUseSeedOverride: %s, SeedOverride: %d, ConfigSeed: %d"), 
		bUseSeedOverride ? TEXT("TRUE") : TEXT("FALSE"), SeedOverride, GeneratorConfig.Seed);

	// Apply Seed Override
	if (bUseSeedOverride)
	{
		GeneratorConfig.Seed = SeedOverride;
		UE_LOG(LogTemp, Warning, TEXT("[Applied] New Seed: %d"), GeneratorConfig.Seed);
	}

	UE_LOG(LogTemp, Warning, TEXT("ADungeonWorldBuilder::Generate() - Config: Width=%d, Height=%d, Seed=%d, Algorithm=%d"),
		GeneratorConfig.Width, GeneratorConfig.Height, GeneratorConfig.Seed, (int)GeneratorConfig.Algorithm);
	
	if (DungeonTheme)
	{
		UE_LOG(LogTemp, Warning, TEXT("ADungeonWorldBuilder::Generate() - Theme: TileSize=%.1f, WallPivotOffset=%s, ThroughWallPivotOffset=%s"),
			DungeonTheme->TileSize, *DungeonTheme->WallPivotOffset.ToString(), *DungeonTheme->ThroughWallPivotOffset.ToString());
	}
	else
	{
		UE_LOG(LogTemp, Error, TEXT("ADungeonWorldBuilder::Generate() - DungeonTheme is NULL!"));
	}

	// 1. Prepare Algorithm
	UDungeonAlgorithm* Algo = nullptr;
	switch (GeneratorConfig.Algorithm)
	{
	case EDungeonAlgorithmType::BSP:
		{
			UBSPGenerator* BSPAlgo = NewObject<UBSPGenerator>(this);
			BSPAlgo->CorridorWidth = GeneratorConfig.CorridorWidth;
			BSPAlgo->SplitRatio = 0.5f; // Add missing property or hardcode
			BSPAlgo->MinRoomSize = GeneratorConfig.MinRoomSize;
			Algo = BSPAlgo;
		}
		break;
	case EDungeonAlgorithmType::CellularAutomata:
		Algo = NewObject<UCellularAutomataGenerator>(this);
		break;
	}

	if (!Algo)
	{
		UE_LOG(LogTemp, Error, TEXT("ADungeonWorldBuilder: Algorithm not selected or failed to create."));
		return;
	}

	// 2. Generate Grid
	FDungeonGrid Grid;
	Grid.Init(GeneratorConfig.Width, GeneratorConfig.Height, ETileType::Wall);
	
	FRandomStream RandomStream(GeneratorConfig.Seed);
	Algo->Generate(Grid, RandomStream);

	// Update BoundsBox to match dungeon size (critical for PCG bounds validation)
	if (BoundsBox && DungeonTheme)
	{
		const float TileSize = DungeonTheme->TileSize;
		const FVector DungeonExtent(
			GeneratorConfig.Width * TileSize * 0.5f,
			GeneratorConfig.Height * TileSize * 0.5f,
			200.0f); // Z-height for dungeon volume
		BoundsBox->SetBoxExtent(DungeonExtent);
		// Position box so that grid (0,0) is at actor origin
		// Box center should be at half the dungeon size
		BoundsBox->SetRelativeLocation(FVector(DungeonExtent.X, DungeonExtent.Y, 0.0f));
		
		UE_LOG(LogTemp, Log, TEXT("DungeonWorldBuilder: TileSize=%.1f, BoundsBox Extent=%s, Location=%s"), 
			TileSize, *DungeonExtent.ToString(), *BoundsBox->GetRelativeLocation().ToString());
	}

	// 3. Render
	if (DungeonRenderer)
	{
		DungeonRenderer->GenerateDungeon(Grid, GeneratorConfig, DungeonTheme, nullptr /* No ChunkStreamer for now */);
	}
}

void ADungeonWorldBuilder::Clear()
{
	if (DungeonRenderer)
	{
		DungeonRenderer->ClearDungeon();
	}
	
	// Destroy spawned Landscape
	if (SpawnedLandscape.IsValid())
	{
		SpawnedLandscape->Destroy();
		SpawnedLandscape.Reset();
		UE_LOG(LogTemp, Log, TEXT("ADungeonWorldBuilder: Landscape destroyed."));
	}
	
	// Cleanup PCG generated actors (they have PCG tag)
	UWorld* World = GetWorld();
	if (World)
	{
		TArray<AActor*> ActorsToDestroy;
		for (TActorIterator<AActor> It(World); It; ++It)
		{
			AActor* Actor = *It;
			if (Actor && Actor->Tags.Contains(FName("PCG Generated Actor")))
			{
				ActorsToDestroy.Add(Actor);
			}
		}
		
		for (AActor* Actor : ActorsToDestroy)
		{
			Actor->Destroy();
		}
		
		if (ActorsToDestroy.Num() > 0)
		{
			UE_LOG(LogTemp, Log, TEXT("ADungeonWorldBuilder: Destroyed %d PCG actors."), ActorsToDestroy.Num());
		}
	}
}

void ADungeonWorldBuilder::GenerateLandscape()
{
#if WITH_EDITOR
	if (OnRequestLandscape.IsBound())
	{
		OnRequestLandscape.Broadcast(this);
	}
	else
	{
		UE_LOG(LogTemp, Warning, TEXT("ADungeonWorldBuilder: Landscape generation tool is not bound. Ensure DungeonGeneratorEditor module is loaded."));
	}
#endif
}
