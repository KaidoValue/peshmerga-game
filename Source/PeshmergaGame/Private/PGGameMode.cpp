#include "PGGameMode.h"
#include "PGCharacter.h"
#include "PGHUD.h"
#include "PGMissionDirector.h"
#include "PGSaveGame.h"
#include "Blueprint/UserWidget.h"
#include "Internationalization/Internationalization.h"

APGGameMode::APGGameMode()
{
	DefaultPawnClass = APGCharacter::StaticClass();
	PlayerControllerClass = APGPlayerController::StaticClass();
	HUDClass = APGHUD::StaticClass();
}

void APGPlayerController::BeginPlay()
{
	Super::BeginPlay();

	if (UPGSaveGame* Save = UPGSaveGame::LoadOrCreate())
	{
		FInternationalization::Get().SetCurrentCulture(Save->Language);
	}

	SetInputMode(FInputModeGameOnly());
	bShowMouseCursor = false;

	if (HUDWidgetClass)
	{
		HUDWidget = CreateWidget<UUserWidget>(this, HUDWidgetClass);
		HUDWidget->AddToViewport();
	}
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		Director->OnChoiceRequested.AddDynamic(this, &APGPlayerController::HandleChoice);
	}
}

void APGPlayerController::HandleChoice(const FText&, const FText&, const FText&)
{
	// Mauszeiger für die Knöpfe im UMG-HUD; die Tasten 1 und 2 funktionieren immer.
	if (HUDWidget)
	{
		bShowMouseCursor = true;
		SetInputMode(FInputModeGameAndUI());
	}
}

void APGPlayerController::PGLanguage(const FString& Culture)
{
	FInternationalization::Get().SetCurrentCulture(Culture);
	if (UPGSaveGame* Save = UPGSaveGame::LoadOrCreate())
	{
		Save->Language = Culture;
		UPGSaveGame::Store(Save);
	}
}
