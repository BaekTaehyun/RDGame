// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CommonActionWidget.h"
#include "RdActionWidget.generated.h"

class UEnhancedInputLocalPlayerSubsystem;
class UInputAction;

/**
 * URdActionWidget
 *
 * An action widget that displays the icon of the key currently assigned to the input action.
 */
UCLASS(BlueprintType, Blueprintable)
class RDGAME_API URdActionWidget : public UCommonActionWidget
{
	GENERATED_BODY()

public:

	//~ Begin UCommonActionWidget interface
	virtual FSlateBrush GetIcon() const override;
	//~ End of UCommonActionWidget interface

	/** The Enhanced Input Action that is associated with this Common Input action. */
	UPROPERTY(BlueprintReadOnly, EditAnywhere, Category = "Input")
	const TObjectPtr<UInputAction> AssociatedInputAction;

private:

	UEnhancedInputLocalPlayerSubsystem* GetEnhancedInputSubsystem() const;
	
};
