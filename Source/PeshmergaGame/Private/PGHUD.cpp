#include "PGHUD.h"
#include "PGCharacter.h"
#include "PGGameMode.h"
#include "PGMissionDirector.h"
#include "Components/Button.h"
#include "Components/TextBlock.h"
#include "Engine/Canvas.h"
#include "Engine/Font.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"

// ---------------------------------------------------------------- Canvas-HUD

void APGHUD::BeginPlay()
{
	Super::BeginPlay();

	// Wenn ein UMG-HUD gesetzt ist, übernimmt das die Anzeige.
	if (const APGPlayerController* PC = Cast<APGPlayerController>(GetOwningPlayerController()))
	{
		bUseCanvas = !PC->HUDWidgetClass;
	}
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		Director->OnObjectiveChanged.AddDynamic(this, &APGHUD::HandleObjective);
		Director->OnSubtitle.AddDynamic(this, &APGHUD::HandleSubtitle);
		Director->OnCaption.AddDynamic(this, &APGHUD::HandleCaption);
		Director->OnChoiceRequested.AddDynamic(this, &APGHUD::HandleChoice);
	}
}

void APGHUD::HandleObjective(const FText& InObjective, AActor* InMarker)
{
	Objective = InObjective;
	Marker = InMarker;
}

void APGHUD::HandleSubtitle(const FText& InSpeaker, const FText& InLine)
{
	Speaker = InSpeaker;
	Line = InLine;
	Choice.Reset();
}

void APGHUD::HandleCaption(const FText& Title, const FText& Sub)
{
	CaptionTitle = Title;
	CaptionSub = Sub;
	CaptionUntil = GetWorld()->GetTimeSeconds() + 5.f;
}

void APGHUD::HandleChoice(const FText& Question, const FText& OptionTell, const FText& OptionSilent)
{
	Choice = { Question, FText::Format(FText::FromString(TEXT("1  {0}")), OptionTell), FText::Format(FText::FromString(TEXT("2  {0}")), OptionSilent) };
}

void APGHUD::DrawCentered(const FString& Text, float Y, float Scale, const FLinearColor& Color)
{
	if (Text.IsEmpty())
	{
		return;
	}
	UFont* Font = GEngine->GetLargeFont();
	float W = 0.f, H = 0.f;
	GetTextSize(Text, W, H, Font, Scale);
	DrawText(Text, FLinearColor(0.f, 0.f, 0.f, 0.6f), (Canvas->SizeX - W) * 0.5f + 2.f, Y + 2.f, Font, Scale);
	DrawText(Text, Color, (Canvas->SizeX - W) * 0.5f, Y, Font, Scale);
}

void APGHUD::DrawHUD()
{
	Super::DrawHUD();
	if (!bUseCanvas || !Canvas)
	{
		return;
	}
	const float S = Canvas->SizeY / 1080.f;
	const FLinearColor Warm(1.f, 0.93f, 0.8f);

	// Ziel oben links
	if (!Objective.IsEmpty())
	{
		DrawText(Objective.ToString(), Warm, 40.f * S, 36.f * S, GEngine->GetLargeFont(), 1.1f * S);
	}

	// Zielmarkierung
	if (AActor* M = Marker.Get())
	{
		FVector2D Screen;
		if (UGameplayStatics::ProjectWorldToScreen(GetOwningPlayerController(), M->GetActorLocation() + FVector(0.f, 0.f, 120.f), Screen))
		{
			DrawRect(FLinearColor(1.f, 0.78f, 0.3f, 0.9f), Screen.X - 5.f * S, Screen.Y - 5.f * S, 10.f * S, 10.f * S);
		}
	}

	// Interaktion
	if (const APGCharacter* Player = Cast<APGCharacter>(GetOwningPawn()))
	{
		DrawCentered(Player->GetFocusPrompt().ToString(), Canvas->SizeY * 0.58f, 1.0f * S, Warm);
		DrawRect(FLinearColor(1.f, 1.f, 1.f, 0.6f), Canvas->SizeX * 0.5f - 1.5f, Canvas->SizeY * 0.5f - 1.5f, 3.f, 3.f);
	}

	// Einblendung (Ort, Zeit)
	if (GetWorld()->GetTimeSeconds() < CaptionUntil)
	{
		DrawCentered(CaptionTitle.ToString(), Canvas->SizeY * 0.30f, 2.0f * S, Warm);
		DrawCentered(CaptionSub.ToString(), Canvas->SizeY * 0.30f + 48.f * S, 1.1f * S, Warm);
	}

	// Untertitel
	if (!Line.IsEmpty())
	{
		DrawCentered(FString::Printf(TEXT("%s: %s"), *Speaker.ToString(), *Line.ToString()), Canvas->SizeY * 0.86f, 1.15f * S, FLinearColor::White);
	}

	// Entscheidung
	for (int32 i = 0; i < Choice.Num(); ++i)
	{
		DrawCentered(Choice[i].ToString(), Canvas->SizeY * (0.40f + 0.07f * i), (i == 0 ? 1.3f : 1.1f) * S, i == 0 ? Warm : FLinearColor::White);
	}
}

// ---------------------------------------------------------------- UMG-HUD

void UPGHUDWidget::NativeConstruct()
{
	Super::NativeConstruct();

	SpeakerText->SetText(FText::GetEmpty());
	SubtitleText->SetText(FText::GetEmpty());
	CaptionTitleText->SetText(FText::GetEmpty());
	CaptionSubText->SetText(FText::GetEmpty());
	if (ChoicePanel)
	{
		ChoicePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (ChoiceTellButton)
	{
		ChoiceTellButton->OnClicked.AddDynamic(this, &UPGHUDWidget::HandleTell);
	}
	if (ChoiceSilentButton)
	{
		ChoiceSilentButton->OnClicked.AddDynamic(this, &UPGHUDWidget::HandleSilent);
	}
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		Director->OnObjectiveChanged.AddDynamic(this, &UPGHUDWidget::HandleObjective);
		Director->OnSubtitle.AddDynamic(this, &UPGHUDWidget::HandleSubtitle);
		Director->OnCaption.AddDynamic(this, &UPGHUDWidget::HandleCaption);
		Director->OnChoiceRequested.AddDynamic(this, &UPGHUDWidget::HandleChoice);
		Director->OnStepChanged.AddDynamic(this, &UPGHUDWidget::HandleStep);
		HandleObjective(Director->GetObjective(), Director->GetObjectiveMarker());
	}
}

void UPGHUDWidget::NativeTick(const FGeometry& MyGeometry, float InDeltaTime)
{
	Super::NativeTick(MyGeometry, InDeltaTime);

	APlayerController* PC = GetOwningPlayer();
	const APGCharacter* Player = PC ? Cast<APGCharacter>(PC->GetPawn()) : nullptr;
	PromptText->SetText(Player ? Player->GetFocusPrompt() : FText::GetEmpty());

	if (GetWorld() && GetWorld()->GetTimeSeconds() > CaptionUntil)
	{
		CaptionTitleText->SetText(FText::GetEmpty());
		CaptionSubText->SetText(FText::GetEmpty());
	}

	if (ObjectiveMarker)
	{
		FVector2D Screen;
		AActor* M = Marker.Get();
		const bool bOnScreen = M && PC && UGameplayStatics::ProjectWorldToScreen(PC, M->GetActorLocation() + FVector(0.f, 0.f, 120.f), Screen, true);
		ObjectiveMarker->SetVisibility(bOnScreen ? ESlateVisibility::HitTestInvisible : ESlateVisibility::Hidden);
		if (bOnScreen)
		{
			ObjectiveMarker->SetRenderTranslation(Screen);
		}
	}
}

void UPGHUDWidget::HandleObjective(const FText& Objective, AActor* InMarker)
{
	ObjectiveText->SetText(Objective);
	Marker = InMarker;
}

void UPGHUDWidget::HandleSubtitle(const FText& Speaker, const FText& Line)
{
	SpeakerText->SetText(Speaker);
	SubtitleText->SetText(Line);
}

void UPGHUDWidget::HandleCaption(const FText& Title, const FText& Subtitle)
{
	CaptionTitleText->SetText(Title);
	CaptionSubText->SetText(Subtitle);
	CaptionUntil = GetWorld()->GetTimeSeconds() + 5.f;
	OnCaptionShown();
}

void UPGHUDWidget::HandleChoice(const FText& Question, const FText& OptionTell, const FText& OptionSilent)
{
	if (ChoiceQuestionText) { ChoiceQuestionText->SetText(Question); }
	if (ChoiceTellText)     { ChoiceTellText->SetText(OptionTell); }
	if (ChoiceSilentText)   { ChoiceSilentText->SetText(OptionSilent); }
	if (ChoicePanel)        { ChoicePanel->SetVisibility(ESlateVisibility::Visible); }
}

void UPGHUDWidget::HandleStep(EPGMissionStep NewStep)
{
	// Auch wenn mit den Tasten 1 oder 2 entschieden wurde, verschwindet das Feld.
	if (ChoicePanel && NewStep != EPGMissionStep::Choice)
	{
		ChoicePanel->SetVisibility(ESlateVisibility::Collapsed);
	}
	if (NewStep != EPGMissionStep::Choice)
	{
		if (APlayerController* PC = GetOwningPlayer())
		{
			PC->bShowMouseCursor = false;
			PC->SetInputMode(FInputModeGameOnly());
		}
	}
}

void UPGHUDWidget::HandleTell()
{
	if (ChoicePanel) { ChoicePanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		Director->MakeChoice(true);
	}
}

void UPGHUDWidget::HandleSilent()
{
	if (ChoicePanel) { ChoicePanel->SetVisibility(ESlateVisibility::Collapsed); }
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		Director->MakeChoice(false);
	}
}
