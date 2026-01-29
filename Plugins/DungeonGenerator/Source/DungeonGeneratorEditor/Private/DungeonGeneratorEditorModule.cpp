#include "DungeonGeneratorEditorModule.h"
#include "DungeonGeneratorTestActorDetails.h"
#include "DungeonFullTestActorDetails.h"
#include "DungeonGeneratorTestActor.h"
#include "DungeonFullTestActor.h"
#include "DungeonLandscapeTool.h"
#include "PropertyEditorModule.h"
#include "DungeonWorldBuilder.h"
#include "ToolMenus.h"

#define LOCTEXT_NAMESPACE "FDungeonGeneratorEditorModule"

void FDungeonGeneratorEditorModule::StartupModule()
{
	// Register Details Customization
	FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
	PropertyModule.RegisterCustomClassLayout(
		ADungeonGeneratorTestActor::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FDungeonGeneratorTestActorDetails::MakeInstance)
	);

	PropertyModule.RegisterCustomClassLayout(
		ADungeonFullTestActor::StaticClass()->GetFName(),
		FOnGetDetailCustomizationInstance::CreateStatic(&FDungeonFullTestActorDetails::MakeInstance)
	);

	// Bind Landscape Tool
	ADungeonWorldBuilder::OnRequestLandscape.AddLambda([](ADungeonWorldBuilder* Actor, const FDungeonGrid* Grid){
		// Call Native version with Grid
		UDungeonLandscapeTool::GenerateLandscapeWithGrid(Actor, true, Grid);
	});

	// Bind Paint Paths Tool
	ADungeonWorldBuilder::OnRequestPaintPath.AddLambda([](ADungeonWorldBuilder* Actor){
		UDungeonLandscapeTool::PaintPaths(Actor);
	});

	// Register MCP Server Button
	UToolMenus::RegisterStartupCallback(FSimpleMulticastDelegate::FDelegate::CreateLambda([]()
	{
		UToolMenu* Menu = UToolMenus::Get()->ExtendMenu("LevelEditor.MainMenu.Tools");
		if (Menu)
		{
			FToolMenuSection& Section = Menu->FindOrAddSection("DungeonGeneratorTools");
			Section.Label = FText::FromString("Dungeon Generator");
			
			Section.AddMenuEntry(
				"StartMCPServer",
				FText::FromString("Start MCP Server"),
				FText::FromString("Launches the Python MCP Server for AI Access."),
				FSlateIcon(),
				FUIAction(FExecuteAction::CreateLambda([]()
				{
					// Execute Python Script
					FString ScriptPath = TEXT("c:/Users/COM2US/Documents/Unreal Projects/RdGame/Script/unreal_mcp_server.py");
					FString Cmd = FString::Printf(TEXT("py \"%s\""), *ScriptPath);
					if (GEngine)
					{
						GEngine->Exec(NULL, *Cmd);
						UE_LOG(LogTemp, Log, TEXT("Started MCP Server: %s"), *Cmd);
					}
				}))
			);
		}
	}));
}

void FDungeonGeneratorEditorModule::ShutdownModule()
{
	if (FModuleManager::Get().IsModuleLoaded("PropertyEditor"))
	{
		FPropertyEditorModule& PropertyModule = FModuleManager::LoadModuleChecked<FPropertyEditorModule>("PropertyEditor");
		PropertyModule.UnregisterCustomClassLayout(ADungeonGeneratorTestActor::StaticClass()->GetFName());
	}
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FDungeonGeneratorEditorModule, DungeonGeneratorEditor)
