#pragma once

#include "CoreMinimal.h"
#include "GameFramework/HUD.h"
#include "Blueprint/UserWidget.h"
#include "PGTypes.h"
#include "PGHUD.generated.h"

class UTextBlock;
class UButton;
class UWidget;

/**
 * Einfaches Text-HUD auf dem Canvas, damit das Projekt ohne Widget-Assets
 * spielbar ist: Ziel, Untertitel, Interaktion, Einblendungen, Entscheidung.
 * Für Sorani bitte UPGHUDWidget verwenden: nur UMG verbindet arabische
 * Buchstaben richtig und schreibt von rechts nach links.
 */
UCLASS()
class PESHMERGAGAME_API APGHUD : public AHUD
{
	GENERATED_BODY()

public:
	virtual void BeginPlay() override;
	virtual void DrawHUD() override;

protected:
	UFUNCTION() void HandleObjective(const FText& Objective, AActor* Marker);
	UFUNCTION() void HandleSubtitle(const FText& Speaker, const FText& Line);
	UFUNCTION() void HandleCaption(const FText& Title, const FText& Subtitle);
	UFUNCTION() void HandleChoice(const FText& Question, const FText& OptionTell, const FText& OptionSilent);

	void DrawCentered(const FString& Text, float Y, float Scale, const FLinearColor& Color);

	FText Objective;
	FText Speaker;
	FText Line;
	FText CaptionTitle;
	FText CaptionSub;
	float CaptionUntil = 0.f;
	TArray<FText> Choice;
	TWeakObjectPtr<AActor> Marker;
	bool bUseCanvas = true;
};

/**
 * Basis für ein UMG-HUD (WBP_HUD). Im Designer Text-Elemente mit genau diesen
 * Namen anlegen. Die Choice-Elemente sind optional.
 */
UCLASS(Abstract)
class PESHMERGAGAME_API UPGHUDWidget : public UUserWidget
{
	GENERATED_BODY()

public:
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> ObjectiveText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SpeakerText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> SubtitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> PromptText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CaptionTitleText;
	UPROPERTY(meta = (BindWidget)) TObjectPtr<UTextBlock> CaptionSubText;

	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ObjectiveMarker;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UWidget> ChoicePanel;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceQuestionText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ChoiceTellButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceTellText;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UButton> ChoiceSilentButton;
	UPROPERTY(meta = (BindWidgetOptional)) TObjectPtr<UTextBlock> ChoiceSilentText;

	/** Im Widget-Blueprint z. B. eine Einblend-Animation abspielen. */
	UFUNCTION(BlueprintImplementableEvent, Category = "HUD")
	void OnCaptionShown();

protected:
	virtual void NativeConstruct() override;
	virtual void NativeTick(const FGeometry& MyGeometry, float InDeltaTime) override;

	UFUNCTION() void HandleObjective(const FText& Objective, AActor* Marker);
	UFUNCTION() void HandleSubtitle(const FText& Speaker, const FText& Line);
	UFUNCTION() void HandleCaption(const FText& Title, const FText& Subtitle);
	UFUNCTION() void HandleChoice(const FText& Question, const FText& OptionTell, const FText& OptionSilent);
	UFUNCTION() void HandleStep(EPGMissionStep NewStep);
	UFUNCTION() void HandleTell();
	UFUNCTION() void HandleSilent();

	TWeakObjectPtr<AActor> Marker;
	float CaptionUntil = 0.f;
};
