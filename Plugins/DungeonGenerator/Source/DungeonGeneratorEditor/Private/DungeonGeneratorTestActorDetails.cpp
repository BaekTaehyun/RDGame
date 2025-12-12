#include "DungeonGeneratorTestActorDetails.h"
#include "DungeonGeneratorTestActor.h"
#include "DetailLayoutBuilder.h"
#include "DetailCategoryBuilder.h"
#include "DetailWidgetRow.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Text/STextBlock.h"

#define LOCTEXT_NAMESPACE "DungeonGeneratorTestActorDetails"

TSharedRef<IDetailCustomization> FDungeonGeneratorTestActorDetails::MakeInstance()
{
	return MakeShareable(new FDungeonGeneratorTestActorDetails);
}

void FDungeonGeneratorTestActorDetails::CustomizeDetails(IDetailLayoutBuilder& DetailBuilder)
{
	IDetailCategoryBuilder& Category = DetailBuilder.EditCategory("Dungeon");

	// Get the selected actor
	TArray<TWeakObjectPtr<UObject>> SelectedObjects;
	DetailBuilder.GetObjectsBeingCustomized(SelectedObjects);

	if (SelectedObjects.Num() == 1)
	{
		SelectedActor = Cast<ADungeonGeneratorTestActor>(SelectedObjects[0].Get());

		if (SelectedActor.IsValid())
		{
			Category.AddCustomRow(LOCTEXT("GenerateButton", "Generate"))
				.NameContent()
				[
					SNew(STextBlock)
					.Text(LOCTEXT("GenerateApprove", "Generator Controls"))
					.Font(IDetailLayoutBuilder::GetDetailFont())
				]
				.ValueContent()
				.MinDesiredWidth(200)
				[
					SNew(SHorizontalBox)
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2.0f)
					[
						SNew(SButton)
						.OnClicked(this, &FDungeonGeneratorTestActorDetails::OnGenerateClicked)
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Generate", "Generate Dungeon"))
							.Justification(ETextJustify::Center)
						]
					]
					+ SHorizontalBox::Slot()
					.FillWidth(1.0f)
					.Padding(2.0f)
					[
						SNew(SButton)
						.OnClicked(this, &FDungeonGeneratorTestActorDetails::OnClearClicked)
						.Content()
						[
							SNew(STextBlock)
							.Text(LOCTEXT("Clear", "Clear Dungeon"))
							.Justification(ETextJustify::Center)
						]
					]
				];
		}
	}
}

FReply FDungeonGeneratorTestActorDetails::OnGenerateClicked()
{
	if (SelectedActor.IsValid())
	{
		// Directly call the Generate function. 
		// Note: The Generate function in the Actor must support Editor-time execution 
		// (e.g. not relying on GameInstanceSubsystem) OR we handle it here.
		// For now, let's call the Actor's function, assuming we will fix it to be editor-safe.
		SelectedActor->Generate();
	}
	return FReply::Handled();
}

FReply FDungeonGeneratorTestActorDetails::OnClearClicked()
{
	if (SelectedActor.IsValid())
	{
		SelectedActor->Clear();
	}
	return FReply::Handled();
}

#undef LOCTEXT_NAMESPACE
