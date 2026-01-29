// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "RdActivatableWidget.h"
#include "Containers/Ticker.h"
#include "GameplayTagContainer.h"

#include "RdHUDLayout.generated.h"

class UCommonActivatableWidget;
class UCommonActivatableWidgetContainerBase;

/**
 * URdHUDLayout
 *
 * The main HUD layout widget for RDGame.
 * This widget serves as the root container for the player's HUD elements.
 * Handles escape menu and controller disconnect screens.
 */
UCLASS(Abstract, BlueprintType, Blueprintable, Meta = (DisplayName = "Rd HUD Layout", Category = "RdGame|HUD"))
class RDGAME_API URdHUDLayout : public URdActivatableWidget
{
	GENERATED_BODY()

public:
	URdHUDLayout(const FObjectInitializer& ObjectInitializer);

	virtual void NativeOnInitialized() override;
	virtual void NativeDestruct() override;

protected:
	/** Called when the escape action is triggered - pushes escape menu to screen */
	void HandleEscapeAction();

	/** 
	 * Callback for when controllers are disconnected. This will check if the player now has 
	 * no mapped input devices to them, which would mean that they can't play the game.
	 */
	void HandleInputDeviceConnectionChanged(EInputDeviceConnectionState NewConnectionState, FPlatformUserId PlatformUserId, FInputDeviceId InputDeviceId);

	/**
	 * Callback for when controllers change their owning platform user.
	 */
	void HandleInputDevicePairingChanged(FInputDeviceId InputDeviceId, FPlatformUserId NewUserPlatformId, FPlatformUserId OldUserPlatformId);

	/**
	 * Notify this widget that the state of controllers for the player have changed.
	 */
	void NotifyControllerStateChangeForDisconnectScreen();

	/**
	 * Check the state of connected controllers and show/hide disconnect menu accordingly.
	 */
	virtual void ProcessControllerDevicesHavingChangedForDisconnectScreen();

	/**
	 * Returns true if this platform supports a "controller disconnected" screen.
	 */
	virtual bool ShouldPlatformDisplayControllerDisconnectScreen() const;

	/**
	 * Pushes the controller disconnected menu to the Menu layer
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Controller Disconnect Menu")
	void DisplayControllerDisconnectedMenu();

	/**
	 * Hides the controller disconnected menu if it is active.
	 */
	UFUNCTION(BlueprintNativeEvent, Category = "Controller Disconnect Menu")
	void HideControllerDisconnectedMenu();

protected:
	/** The menu to be displayed when the user presses the "Pause" or "Escape" button */
	UPROPERTY(EditDefaultsOnly, Category = "Escape Menu")
	TSoftClassPtr<UCommonActivatableWidget> EscapeMenuClass;

	/** The widget which should be presented to the user if all of their controllers are disconnected. */
	UPROPERTY(EditDefaultsOnly, Category = "Controller Disconnect Menu")
	TSubclassOf<UCommonActivatableWidget> ControllerDisconnectedScreen;

	/**
	 * The platform tags that are required in order to show the "Controller Disconnected" screen.
	 * If these tags are not set in the INI file for this platform, then the controller disconnect screen
	 * will not ever be displayed.
	 */
	UPROPERTY(EditDefaultsOnly, Category = "Controller Disconnect Menu")
	FGameplayTagContainer PlatformRequiresControllerDisconnectScreen;

	/** Pointer to the active "Controller Disconnected" menu if there is one. */
	UPROPERTY(Transient)
	TObjectPtr<UCommonActivatableWidget> SpawnedControllerDisconnectScreen;

	/** Handle from the FSTicker for when we want to process the controller state of our player */
	FTSTicker::FDelegateHandle RequestProcessControllerStateHandle;

	/** Menu layer for pause menus, settings, etc. (optional bind widget) */
	UPROPERTY(BlueprintReadOnly, meta = (BindWidget, OptionalWidget = true), Category = "UI")
	TObjectPtr<UCommonActivatableWidgetContainerBase> MenuLayer;
};

