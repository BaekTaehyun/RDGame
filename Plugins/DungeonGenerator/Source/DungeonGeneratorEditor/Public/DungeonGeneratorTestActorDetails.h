#pragma once

#include "CoreMinimal.h"
#include "IDetailCustomization.h"

class FDungeonGeneratorTestActorDetails : public IDetailCustomization
{
public:
	/** Makes a new instance of this detail layout class for a specific detail view requesting it */
	static TSharedRef<IDetailCustomization> MakeInstance();

	/** IDetailCustomization interface */
	virtual void CustomizeDetails(IDetailLayoutBuilder& DetailBuilder) override;

	/** Callback for the Generate button */
	FReply OnGenerateClicked();

	/** Callback for the Clear button */
	FReply OnClearClicked();

private:
	/** Weak reference to the selected actor(s) */
	TWeakObjectPtr<class ADungeonGeneratorTestActor> SelectedActor;
};
