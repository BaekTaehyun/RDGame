#include "DungeonGeneratorEditorModule.h"
#include "DungeonGeneratorTestActorDetails.h"
#include "DungeonFullTestActorDetails.h"
#include "DungeonGeneratorTestActor.h"
#include "DungeonFullTestActor.h"
#include "PropertyEditorModule.h"

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
