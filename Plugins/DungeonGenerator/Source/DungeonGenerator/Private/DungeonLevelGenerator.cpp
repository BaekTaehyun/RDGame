#include "DungeonLevelGenerator.h"
#include "Components/DungeonRendererComponent.h"
#include "Rendering/DungeonChunkStreamer.h"
#include "Generation/DungeonBuilder.h"
#include "Data/DungeonThemeAsset.h"

ADungeonLevelGenerator::ADungeonLevelGenerator()
{
	PrimaryActorTick.bCanEverTick = false;

	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));

	DungeonRenderer = CreateDefaultSubobject<UDungeonRendererComponent>(TEXT("DungeonRenderer"));
	// Component doesn't needed to be attached effectively as it just manages other components, 
    // but usually we don't attach ActorComponents to SceneHierarchy unless they are SceneComponents.
    // UDungeonRendererComponent inherits UActorComponent, not USceneComponent. Correct.
    
	ChunkStreamer = CreateDefaultSubobject<UDungeonChunkStreamer>(TEXT("ChunkStreamer"));
}

void ADungeonLevelGenerator::BeginPlay()
{
	Super::BeginPlay();
    
    // In Game Runtime (PIE/Standalone), we generate if needed.
    // But usually BeginPlay runs ONCE.
    // If we loaded from disk, everything might be there.
    // However, for procedural dungeons, we often want fresh generation or restore.
    // Logic: If Config Valid -> Generate.
    
    // BUT: If PostLoad (Editor) already generated, do we generate again?
    // In PIE, PostLoad runs before BeginPlay?
    // If PostLoad skipped regen (PIE), then we MUST rely on `RebuildChunkMaps` which `HandlePostLoad` did.
    // `DungeonFullTestActor::BeginPlay` called `RebuildChunkMaps` again just in case, and `UpdateActiveChunks`.
    
    // Let's act like DungeonFullTestActor:
    // 1. Rebuild Maps (Safety)
    if (DungeonRenderer)
    {
        DungeonRenderer->RebuildChunkMaps();
    }

    // 2. Streamer Init
	if (ChunkStreamer && DungeonRenderer)
	{
        // Setup Streamer Props from Config?
        FDungeonGenConfig Config;
        if (GetDungeonConfig(Config))
        {
            ChunkStreamer->bEnableStreaming = Config.bEnableStreaming;
            ChunkStreamer->StreamingDistance = Config.StreamingDistance;
            ChunkStreamer->UpdateInterval = Config.StreamingUpdateInterval;
            
            // Need TileSize/ChunkSize
            if (ThemeOverride) 
            {
                 ChunkStreamer->TileSize = ThemeOverride->TileSize;
            }
            else if (Config.Theme.IsValid())
            {
                // Load Theme to get TileSize
                 UDungeonThemeAsset* LoadedTheme = Config.Theme.LoadSynchronous();
                 if (LoadedTheme) ChunkStreamer->TileSize = LoadedTheme->TileSize;
            }
            ChunkStreamer->ChunkSize = Config.ChunkSize;
        }

        // Push Data
		ChunkStreamer->ChunkHISMMap = DungeonRenderer->ChunkHISMMap;
        ChunkStreamer->ChunkMergedMeshMap = DungeonRenderer->MergedChunkMeshes;
        
        // Initial Update
		ChunkStreamer->UpdateActiveChunks(GetActorLocation());
	}
}

void ADungeonLevelGenerator::PostLoad()
{
	Super::PostLoad();

    bool bIsPIE = (GetPackage()->HasAnyPackageFlags(PKG_PlayInEditor));

    if (DungeonRenderer)
    {
        DungeonRenderer->HandlePostLoad(bIsPIE);
    }

    // "Missing Walls in Editor" Fix: do NOT regenerate here.
    // PostLoad cannot register new components safely (causes Ensure failure).
    // Actors should rely on manual Generation or Construction Scripts.
}

void ADungeonLevelGenerator::OnConstruction(const FTransform& Transform)
{
	Super::OnConstruction(Transform);

	// Ensure Components are valid before asking them to do work
	if (DungeonRenderer)
	{
		// In Editor, simple property changes trigger Construction.
		// We might want to Auto-Generate if the Config Row is valid.
		// NOTE: Be careful of infinite loops if GenerateDungeon modifies properties that trigger Construction.
		// Our GenerateDungeon mainly spawns components, which might trigger OnConstruction again if they attach?
		// Usually spawning attached actors triggers it, but new components should be fine if transient.
		
		// Only auto-generate if we have a valid config row selected
		if (!DungeonConfigRow.IsNull())
		{
             // Optional: Check a bool "bAutoGenerate" if we want to suppress it
             GenerateDungeon();
		}
	}
}

void ADungeonLevelGenerator::GenerateDungeon()
{
    // 1. Resolve Config
    FDungeonGenConfig Config;
    if (!GetDungeonConfig(Config))
    {
        UE_LOG(LogTemp, Warning, TEXT("ADungeonLevelGenerator: Invalid Config Row"));
        return;
    }

    // 2. Resolve Theme
    const UDungeonThemeAsset* TargetTheme = ThemeOverride;
    if (!TargetTheme)
    {
        if (Config.Theme.IsNull())
        {
            UE_LOG(LogTemp, Warning, TEXT("ADungeonLevelGenerator: No Theme assigned in Config"));
            return;
        }
        TargetTheme = Config.Theme.LoadSynchronous();
    }
    
    if (!TargetTheme)
    {
        UE_LOG(LogTemp, Error, TEXT("ADungeonLevelGenerator: Failed to load Theme"));
        return;
    }

    // 3. Build Logic
    UE_LOG(LogTemp, Log, TEXT("ADungeonLevelGenerator: Building Dungeon (Seed: %d)"), Config.Seed);
    FDungeonGrid Grid = UDungeonBuilder::BuildDungeon(Config, this);

    // 4. Render
    if (DungeonRenderer)
    {
        DungeonRenderer->GenerateDungeon(Grid, Config, TargetTheme, ChunkStreamer);
    }
}

void ADungeonLevelGenerator::ClearDungeon()
{
    if (DungeonRenderer)
    {
        DungeonRenderer->ClearDungeon();
    }
}

bool ADungeonLevelGenerator::GetDungeonConfig(FDungeonGenConfig& OutConfig)
{
    if (DungeonConfigRow.IsNull()) return false;

    // GetDataTableRow is generic, requires row name and table
    // FDataTableRowHandle has GetRow<T>() helper
    FDungeonGenConfig* Row = DungeonConfigRow.GetRow<FDungeonGenConfig>(TEXT("DungeonGeneratorContext"));
    
    if (Row)
    {
        OutConfig = *Row;
        return true;
    }
    return false;
}
