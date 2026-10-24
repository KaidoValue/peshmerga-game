#include "PGMissionDirector.h"
#include "PeshmergaGame.h"
#include "PGCharacter.h"
#include "PGGuard.h"
#include "PGInteractableActor.h"
#include "PGReconDrone.h"
#include "PGSaveGame.h"
#include "Camera/CameraShakeBase.h"
#include "Components/LightComponent.h"
#include "Engine/DataTable.h"
#include "Engine/DirectionalLight.h"
#include "EngineUtils.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "Sound/SoundBase.h"
#include "TimerManager.h"
#include "Engine/Engine.h"
#include "Engine/World.h"

APGMissionDirector::APGMissionDirector()
{
	PrimaryActorTick.bCanEverTick = true;
	SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
}

APGMissionDirector* APGMissionDirector::Get(const UObject* WorldContext)
{
	UWorld* World = GEngine ? GEngine->GetWorldFromContextObject(WorldContext, EGetWorldErrorMode::ReturnNull) : nullptr;
	if (!World)
	{
		return nullptr;
	}
	TActorIterator<APGMissionDirector> It(World);
	return It ? *It : nullptr;
}

void APGMissionDirector::BeginPlay()
{
	Super::BeginPlay();
	StartTime = GetWorld()->GetTimeSeconds();

	if (Drone)
	{
		Drone->SetActive(false);
	}
	if (Guard)
	{
		Guard->SetActive(false);
	}
	if (DialogueTable)
	{
		// Die vier Sätze am Satellitentelefon werden einzeln abgespielt, solange man lauscht.
		TArray<FName> Names = DialogueTable->GetRowNames();
		Names.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
		for (const FName& Name : Names)
		{
			if (Name.ToString().StartsWith(TEXT("M01_listen_")))
			{
				ListenLines.Add(DialogueTable->FindRow<FPGDialogueLine>(Name, TEXT("Listen")));
			}
		}
	}

	SetTimeOfDay(TEXT("Day"));
	SetStep(EPGMissionStep::Intro);

	// Einen Frame warten, damit HUD und Widgets ihre Ereignisse schon abonniert haben.
	GetWorldTimerManager().SetTimerForNextTick([this]()
	{
		ShowCaption(TEXT("capIntro"), TEXT("capIntroSub"));
		FTimerHandle IntroTimer;
		GetWorldTimerManager().SetTimer(IntroTimer, [this]()
		{
			SetStep(EPGMissionStep::Water);
			SetObjective(TEXT("oWater"), Spring);
		}, 4.5f, false);
	});
}

void APGMissionDirector::EndPlay(const EEndPlayReason::Type Reason)
{
	GetWorldTimerManager().ClearAllTimersForObject(this);
	Super::EndPlay(Reason);
}

// ---------------------------------------------------------------- Texte

FText APGMissionDirector::GetUIText(FName Key) const
{
	if (UITextTable)
	{
		if (const FPGUIText* Row = UITextTable->FindRow<FPGUIText>(Key, TEXT("UIText"), false))
		{
			return PGText::Pick(Row->TextDE, Row->TextCKB);
		}
	}
	return FText::FromName(Key);
}

void APGMissionDirector::SetObjective(FName Key, AActor* Marker)
{
	ObjectiveText = Key.IsNone() ? FText::GetEmpty() : GetUIText(Key);
	ObjectiveMarker = Marker;
	OnObjectiveChanged.Broadcast(ObjectiveText, Marker);
}

void APGMissionDirector::ShowCaption(FName TitleKey, FName SubKey)
{
	OnCaption.Broadcast(GetUIText(TitleKey), SubKey.IsNone() ? FText::GetEmpty() : GetUIText(SubKey));
}

void APGMissionDirector::SetStep(EPGMissionStep NewStep)
{
	Step = NewStep;
	UE_LOG(LogPeshmerga, Log, TEXT("Mission 01: Schritt %s"), *UEnum::GetValueAsString(NewStep));
	OnStepChanged.Broadcast(NewStep);
}

// ---------------------------------------------------------------- Untertitel

void APGMissionDirector::PlayGroup(const FString& Group, TFunction<void()> OnDone)
{
	LineQueue.Reset();
	if (DialogueTable)
	{
		const FString Prefix = FString::Printf(TEXT("M01_%s_"), *Group);
		TArray<FName> Names = DialogueTable->GetRowNames();
		Names.Sort([](const FName& A, const FName& B) { return A.LexicalLess(B); });
		for (const FName& Name : Names)
		{
			if (Name.ToString().StartsWith(Prefix))
			{
				LineQueue.Add(DialogueTable->FindRow<FPGDialogueLine>(Name, TEXT("Dialogue")));
			}
		}
	}
	QueueDone = MoveTemp(OnDone);
	CurrentLine = INDEX_NONE;
	PlayNextLine();
}

void APGMissionDirector::PlayNextLine()
{
	++CurrentLine;
	if (!LineQueue.IsValidIndex(CurrentLine))
	{
		CurrentLine = INDEX_NONE;
		LineQueue.Reset();
		OnSubtitle.Broadcast(FText::GetEmpty(), FText::GetEmpty());
		if (QueueDone)
		{
			TFunction<void()> Done = MoveTemp(QueueDone);
			QueueDone = nullptr;
			Done();
		}
		return;
	}

	const FPGDialogueLine* Line = LineQueue[CurrentLine];
	float Duration = Line ? Line->Duration : 1.f;
	if (Line)
	{
		OnSubtitle.Broadcast(PGText::Pick(Line->SpeakerDE, Line->SpeakerCKB), PGText::Pick(Line->TextDE, Line->TextCKB));
		if (USoundBase* Voice = Line->Voice.LoadSynchronous())
		{
			UGameplayStatics::PlaySound2D(this, Voice);
			Duration = FMath::Max(Duration, Voice->GetDuration() + 0.3f);
		}
	}
	GetWorldTimerManager().SetTimer(LineTimer, this, &APGMissionDirector::FinishLine, Duration, false);
}

void APGMissionDirector::FinishLine()
{
	PlayNextLine();
}

void APGMissionDirector::SkipLine()
{
	if (IsSpeaking())
	{
		GetWorldTimerManager().ClearTimer(LineTimer);
		FinishLine();
	}
}

// ---------------------------------------------------------------- Ablauf

void APGMissionDirector::NotifyInteracted(APGInteractableActor* Actor, APGCharacter* Player)
{
	switch (Step)
	{
	case EPGMissionStep::Water:
		if (Actor == Spring)
		{
			SetStep(EPGMissionStep::Tea);
			SetObjective(TEXT("oTea"), Dara);
		}
		break;

	case EPGMissionStep::Tea:
		if (Actor == Dara)
		{
			SetObjective(NAME_None, nullptr);
			PlayGroup(TEXT("tea"), [this]()
			{
				SetStep(EPGMissionStep::Radio);
				SetObjective(TEXT("oRadio"), Radio ? Radio.Get() : Azad.Get());
			});
		}
		break;

	case EPGMissionStep::Radio:
		if (Actor == Radio || Actor == Azad)
		{
			SetObjective(NAME_None, nullptr);
			PlayGroup(TEXT("radio"), [this]() { StartAttack(); });
		}
		break;

	case EPGMissionStep::Return:
		if (Actor == Dara)
		{
			StartChoice();
		}
		break;

	default:
		break;
	}
}

void APGMissionDirector::StartAttack()
{
	SetStep(EPGMissionStep::Attack);
	ImpactIndex = 0;
	// Kurze Stille nach dem Funkspruch, dann die Einschläge.
	GetWorldTimerManager().SetTimer(ImpactTimer, this, &APGMissionDirector::DoImpact, 6.f, false);
}

void APGMissionDirector::DoImpact()
{
	if (!ImpactPoints.IsValidIndex(ImpactIndex))
	{
		return;
	}
	AActor* Point = ImpactPoints[ImpactIndex];
	if (Point)
	{
		const FVector Loc = Point->GetActorLocation();
		if (ExplosionFX)
		{
			UNiagaraFunctionLibrary::SpawnSystemAtLocation(this, ExplosionFX, Loc);
		}
		if (ExplosionSound)
		{
			UGameplayStatics::PlaySoundAtLocation(this, ExplosionSound, Loc);
		}
		if (ImpactShake)
		{
			UGameplayStatics::PlayWorldCameraShake(this, ImpactShake, Loc, 0.f, 6000.f);
		}
		OnImpact(ImpactIndex, Point);
	}

	if (ImpactIndex == 0)
	{
		// Erster Einschlag: Tinnitus, gedämpftes Hören, Dara ruft zur Höhle.
		if (TinnitusSound)
		{
			UGameplayStatics::PlaySound2D(this, TinnitusSound);
		}
		if (MuffleMix)
		{
			UGameplayStatics::PushSoundMixModifier(this, MuffleMix);
			FTimerHandle MuffleTimer;
			GetWorldTimerManager().SetTimer(MuffleTimer, [this]()
			{
				UGameplayStatics::PopSoundMixModifier(this, MuffleMix);
			}, 9.f, false);
		}
		for (AActor* A : HideAfterAttack)
		{
			if (A)
			{
				A->SetActorHiddenInGame(true);
				A->SetActorEnableCollision(false);
			}
		}
		SetStep(EPGMissionStep::Cave);
		PlayGroup(TEXT("attack"));
		SetObjective(TEXT("oCave"), CaveMarker);
	}

	++ImpactIndex;
	if (ImpactPoints.IsValidIndex(ImpactIndex))
	{
		GetWorldTimerManager().SetTimer(ImpactTimer, this, &APGMissionDirector::DoImpact, ImpactIndex == 1 ? 2.2f : 3.1f, false);
	}
}

void APGMissionDirector::NotifyTrigger(FName TriggerId, APGCharacter* Player, bool bEntered)
{
	if (TriggerId == TEXT("Cave"))
	{
		bInCave = bEntered;
		if (!bEntered)
		{
			return;
		}
		if (Step == EPGMissionStep::Cave)
		{
			SetStep(EPGMissionStep::Wounded);
			SetObjective(NAME_None, nullptr);
			PlayGroup(TEXT("cave"), [this]() { StartWounded(); });
		}
		else if (Step == EPGMissionStep::Wounded && Player && Player->IsCarrying())
		{
			if (APGInteractableActor* W = Player->StopCarry())
			{
				W->bRescued = true;
				if (CaveDropPoints.IsValidIndex(SavedCount) && CaveDropPoints[SavedCount])
				{
					W->SetActorTransform(CaveDropPoints[SavedCount]->GetActorTransform());
				}
				++SavedCount;
				UpdateWoundedObjective();
				if (SavedCount >= Wounded.Num())
				{
					StartEscape();
				}
			}
		}
	}
	else if (TriggerId == TEXT("Hut") && bEntered && Step == EPGMissionStep::Escape)
	{
		StartNight();
	}
	else if (TriggerId == TEXT("Listen"))
	{
		bInListenZone = bEntered;
	}
}

void APGMissionDirector::StartWounded()
{
	WoundedTimeLeft = WoundedTimeSeconds;
	UpdateWoundedObjective();
}

void APGMissionDirector::UpdateWoundedObjective()
{
	FFormatNamedArguments Args;
	Args.Add(TEXT("Saved"), FText::AsNumber(SavedCount));
	Args.Add(TEXT("Seconds"), FText::AsNumber(FMath::CeilToInt(WoundedTimeLeft)));
	ObjectiveText = FText::Format(GetUIText(TEXT("oWounded")), Args);

	// Markierung auf den nächsten Verwundeten, der noch draußen liegt.
	AActor* Next = nullptr;
	if (const APGCharacter* Player = GetPlayer(); Player && Player->IsCarrying())
	{
		Next = CaveMarker;
	}
	else
	{
		for (APGInteractableActor* W : Wounded)
		{
			if (W && !W->bRescued)
			{
				Next = W;
				break;
			}
		}
	}
	ObjectiveMarker = Next;
	OnObjectiveChanged.Broadcast(ObjectiveText, Next);
}

void APGMissionDirector::StartEscape()
{
	if (Step != EPGMissionStep::Wounded)
	{
		return;
	}
	// Wer nicht gerettet wurde, bleibt zurück. Das Spiel merkt es sich.
	if (APGCharacter* Player = GetPlayer())
	{
		Player->StopCarry();
	}
	SetStep(EPGMissionStep::Escape);
	SetObjective(NAME_None, nullptr);
	PlayGroup(TEXT("escape"), [this]()
	{
		if (Drone)
		{
			Drone->SetActive(true);
		}
		SetObjective(TEXT("oEscape"), HutMarker);
	});
}

void APGMissionDirector::NotifyDetected(FName CaptionKey)
{
	if (bFailing)
	{
		return;
	}
	bFailing = true;
	ShowCaption(CaptionKey, NAME_None);
	AActor* Point = (Step == EPGMissionStep::Escape) ? EscapeCheckpoint.Get() : NightPlayerStart.Get();

	FTimerHandle RespawnTimer;
	GetWorldTimerManager().SetTimer(RespawnTimer, [this, Point]()
	{
		RespawnAt(Point);
		bFailing = false;
	}, 2.5f, false);
}

void APGMissionDirector::RespawnAt(AActor* Point)
{
	APGCharacter* Player = GetPlayer();
	if (!Player || !Point)
	{
		return;
	}
	Player->TeleportTo(Point->GetActorLocation(), Point->GetActorRotation());
	if (APlayerController* PC = Cast<APlayerController>(Player->GetController()))
	{
		PC->SetControlRotation(Point->GetActorRotation());
	}
	if (Drone)
	{
		Drone->ResetDetection();
	}
	if (Guard)
	{
		Guard->ResetDetection();
	}
}

void APGMissionDirector::StartNight()
{
	SetStep(EPGMissionStep::Night);
	SetObjective(NAME_None, nullptr);
	if (Drone)
	{
		Drone->SetActive(false);
	}

	APGCharacter* Player = GetPlayer();
	if (APlayerController* PC = Player ? Cast<APlayerController>(Player->GetController()) : nullptr)
	{
		PC->PlayerCameraManager->StartCameraFade(0.f, 1.f, 1.2f, FLinearColor::Black, false, true);
	}

	FTimerHandle NightTimer;
	GetWorldTimerManager().SetTimer(NightTimer, [this]()
	{
		SetTimeOfDay(TEXT("Night"));
		if (Dara && NightDaraPoint)
		{
			Dara->SetActorTransform(NightDaraPoint->GetActorTransform());
		}
		RespawnAt(NightPlayerStart);
		if (APGCharacter* P = GetPlayer())
		{
			if (APlayerController* PC = Cast<APlayerController>(P->GetController()))
			{
				PC->PlayerCameraManager->StartCameraFade(1.f, 0.f, 2.5f, FLinearColor::Black);
			}
		}
		ShowCaption(TEXT("capNight"), TEXT("capNightSub"));
		PlayGroup(TEXT("night"), [this]()
		{
			SetStep(EPGMissionStep::Listen);
			SetObjective(TEXT("oListen"), ListenMarker);
			ListenIndex = 0;
			if (Guard)
			{
				Guard->SetActive(true);
			}
		});
	}, 1.4f, false);
}

void APGMissionDirector::StartChoice()
{
	SetStep(EPGMissionStep::Choice);
	SetObjective(NAME_None, nullptr);
	if (Guard)
	{
		Guard->SetActive(false);
	}
	if (APGCharacter* Player = GetPlayer())
	{
		Player->SetControlsLocked(true);
	}
	OnChoiceRequested.Broadcast(GetUIText(TEXT("q")), GetUIText(TEXT("c1")), GetUIText(TEXT("c2")));
}

void APGMissionDirector::MakeChoice(bool bTellDara)
{
	if (Step != EPGMissionStep::Choice)
	{
		return;
	}
	bToldDara = bTellDara;
	SetStep(EPGMissionStep::Dawn);
	PlayGroup(bTellDara ? TEXT("tell") : TEXT("silent"), [this]()
	{
		SetTimeOfDay(TEXT("Dawn"));
		ShowCaption(TEXT("capDawn"), TEXT("capDawnSub"));
		FTimerHandle EndTimer;
		GetWorldTimerManager().SetTimer(EndTimer, this, &APGMissionDirector::FinishMission, 6.f, false);
	});
}

void APGMissionDirector::FinishMission()
{
	SetStep(EPGMissionStep::Finished);
	const float PlayTime = GetWorld()->GetTimeSeconds() - StartTime;

	UPGSaveGame* Save = UPGSaveGame::LoadOrCreate();
	Save->bM01Completed = true;
	Save->bM01ToldDara = bToldDara;
	Save->M01SavedWounded = SavedCount;
	UPGSaveGame::Store(Save);

	ShowCaption(TEXT("endTitle"), TEXT("endText"));
	OnMissionFinished.Broadcast(bToldDara, SavedCount, PlayTime);
}

void APGMissionDirector::SetTimeOfDay(FName Phase)
{
	if (Sun)
	{
		// Pitch der Sonne: Tag hoch im Osten, Nacht unter dem Horizont (Mondlicht schwach), Morgengrauen knapp darüber.
		float Pitch = -32.f, Yaw = 120.f, Intensity = 9.f;
		if (Phase == TEXT("Night"))     { Pitch = 25.f; Yaw = 300.f; Intensity = 0.15f; }
		else if (Phase == TEXT("Dawn")) { Pitch = -4.f; Yaw = 95.f;  Intensity = 3.5f; }
		Sun->SetActorRotation(FRotator(Pitch, Yaw, 0.f));
		if (ULightComponent* Light = Sun->GetLightComponent())
		{
			Light->SetIntensity(Intensity);
		}
	}
	OnTimeOfDay(Phase);
}

APGCharacter* APGMissionDirector::GetPlayer() const
{
	return Cast<APGCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));
}

void APGMissionDirector::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	if (Step == EPGMissionStep::Wounded && !IsSpeaking() && WoundedTimeLeft > 0.f)
	{
		const int32 Before = FMath::CeilToInt(WoundedTimeLeft);
		WoundedTimeLeft -= DeltaSeconds;
		if (WoundedTimeLeft <= 0.f)
		{
			StartEscape();
		}
		else if (FMath::CeilToInt(WoundedTimeLeft) != Before)
		{
			UpdateWoundedObjective();
		}
	}

	// Lauschen: nur geduckt hinter Hütte B geht das Gespräch weiter.
	if (Step == EPGMissionStep::Listen && !IsSpeaking() && !bFailing)
	{
		const APGCharacter* Player = GetPlayer();
		if (bInListenZone && Player && Player->bIsCrouched)
		{
			if (ListenLines.IsValidIndex(ListenIndex))
			{
				LineQueue = { ListenLines[ListenIndex] };
				CurrentLine = INDEX_NONE;
				++ListenIndex;
				QueueDone = nullptr;
				PlayNextLine();
			}
			else
			{
				SetStep(EPGMissionStep::Return);
				SetObjective(TEXT("oReturn"), Dara);
			}
		}
	}
}
