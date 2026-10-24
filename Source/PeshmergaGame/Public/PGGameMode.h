#pragma once

#include "CoreMinimal.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/PlayerController.h"
#include "PGGameMode.generated.h"

class UUserWidget;

UCLASS()
class PESHMERGAGAME_API APGGameMode : public AGameModeBase
{
	GENERATED_BODY()

public:
	APGGameMode();
};

UCLASS()
class PESHMERGAGAME_API APGPlayerController : public APlayerController
{
	GENERATED_BODY()

public:
	/** Optionales UMG-HUD (Unterklasse von UPGHUDWidget). Ohne Widget zeichnet APGHUD einen einfachen Text-HUD. */
	UPROPERTY(EditDefaultsOnly, Category = "HUD")
	TSubclassOf<UUserWidget> HUDWidgetClass;

	/** Konsole: PGLanguage ckb  oder  PGLanguage de */
	UFUNCTION(Exec, BlueprintCallable, Category = "Sprache")
	void PGLanguage(const FString& Culture);

protected:
	virtual void BeginPlay() override;

	UPROPERTY(Transient)
	TObjectPtr<UUserWidget> HUDWidget;

	UFUNCTION()
	void HandleChoice(const FText& Question, const FText& OptionTell, const FText& OptionSilent);
};
