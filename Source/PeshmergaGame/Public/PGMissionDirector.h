#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PGTypes.h"
#include "PGMissionDirector.generated.h"

class APGCharacter;
class APGInteractableActor;
class APGReconDrone;
class APGGuard;
class ADirectionalLight;
class UDataTable;
class UNiagaraSystem;
class USoundBase;
class USoundMix;
class UCameraShakeBase;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPGOnObjective, const FText&, Objective, AActor*, Marker);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPGOnSubtitle, const FText&, Speaker, const FText&, Line);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPGOnCaption, const FText&, Title, const FText&, Subtitle);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPGOnChoice, const FText&, Question, const FText&, OptionTell, const FText&, OptionSilent);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_ThreeParams(FPGOnFinished, bool, bToldDara, int32, SavedWounded, float, PlayTimeSeconds);
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FPGOnStep, EPGMissionStep, NewStep);

/**
 * Ablauf von Mission 01 "Feuer über Qandil".
 * Einmal in die Karte ziehen und die Verweise im Details-Panel setzen.
 * Ablauf: Wasser -> Tee -> Funk -> Angriff -> Höhle -> Verwundete -> Flucht -> Nacht -> Lauschen -> Rückkehr -> Entscheidung -> Morgengrauen.
 */
UCLASS()
class PESHMERGAGAME_API APGMissionDirector : public AActor
{
	GENERATED_BODY()

public:
	APGMissionDirector();

	/** Der Director der aktuellen Welt (oder nullptr). */
	static APGMissionDirector* Get(const UObject* WorldContext);

	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
	virtual void Tick(float DeltaSeconds) override;

	// ---------- Daten ----------
	UPROPERTY(EditAnywhere, Category = "Mission|Daten")
	TObjectPtr<UDataTable> DialogueTable;

	UPROPERTY(EditAnywhere, Category = "Mission|Daten")
	TObjectPtr<UDataTable> UITextTable;

	// ---------- Figuren und Objekte in der Karte ----------
	UPROPERTY(EditInstanceOnly, Category = "Mission|Lager")
	TObjectPtr<APGInteractableActor> Spring;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Lager")
	TObjectPtr<APGInteractableActor> Dara;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Lager")
	TObjectPtr<APGInteractableActor> Azad;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Lager")
	TObjectPtr<APGInteractableActor> Radio;

	/** Cemal, Baran, Kawa und andere Figuren, die nach dem Angriff verschwinden. */
	UPROPERTY(EditInstanceOnly, Category = "Mission|Lager")
	TArray<TObjectPtr<AActor>> HideAfterAttack;

	/** Orte der drei Einschläge (Hütten). Der Angreifer wird nie gezeigt. */
	UPROPERTY(EditInstanceOnly, Category = "Mission|Angriff")
	TArray<TObjectPtr<AActor>> ImpactPoints;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Angriff")
	TObjectPtr<AActor> CaveMarker;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Angriff")
	TArray<TObjectPtr<APGInteractableActor>> Wounded;

	/** Wo die geretteten Verwundeten in der Höhle liegen. */
	UPROPERTY(EditInstanceOnly, Category = "Mission|Angriff")
	TArray<TObjectPtr<AActor>> CaveDropPoints;

	UPROPERTY(EditAnywhere, Category = "Mission|Angriff", meta = (ClampMin = "30"))
	float WoundedTimeSeconds = 150.f;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Flucht")
	TObjectPtr<APGReconDrone> Drone;

	/** Startpunkt nach Entdeckung durch die Drohne (Höhlenausgang). */
	UPROPERTY(EditInstanceOnly, Category = "Mission|Flucht")
	TObjectPtr<AActor> EscapeCheckpoint;

	/** Hirtenhütte A: Ziel der Flucht. */
	UPROPERTY(EditInstanceOnly, Category = "Mission|Flucht")
	TObjectPtr<AActor> HutMarker;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Nacht")
	TObjectPtr<AActor> NightPlayerStart;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Nacht")
	TObjectPtr<AActor> NightDaraPoint;

	/** Platz hinter Hütte B, an dem man Cemal hört. */
	UPROPERTY(EditInstanceOnly, Category = "Mission|Nacht")
	TObjectPtr<AActor> ListenMarker;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Nacht")
	TObjectPtr<APGGuard> Guard;

	UPROPERTY(EditInstanceOnly, Category = "Mission|Licht")
	TObjectPtr<ADirectionalLight> Sun;

	// ---------- Effekte ----------
	UPROPERTY(EditAnywhere, Category = "Mission|Effekte")
	TObjectPtr<UNiagaraSystem> ExplosionFX;

	UPROPERTY(EditAnywhere, Category = "Mission|Effekte")
	TObjectPtr<USoundBase> ExplosionSound;

	UPROPERTY(EditAnywhere, Category = "Mission|Effekte")
	TObjectPtr<USoundBase> TinnitusSound;

	/** Tiefpass nach dem Einschlag (gedämpftes Hören). */
	UPROPERTY(EditAnywhere, Category = "Mission|Effekte")
	TObjectPtr<USoundMix> MuffleMix;

	UPROPERTY(EditAnywhere, Category = "Mission|Effekte")
	TSubclassOf<UCameraShakeBase> ImpactShake;

	// ---------- Ereignisse für HUD und Level-Blueprint ----------
	UPROPERTY(BlueprintAssignable) FPGOnObjective OnObjectiveChanged;
	UPROPERTY(BlueprintAssignable) FPGOnSubtitle OnSubtitle;
	UPROPERTY(BlueprintAssignable) FPGOnCaption OnCaption;
	UPROPERTY(BlueprintAssignable) FPGOnChoice OnChoiceRequested;
	UPROPERTY(BlueprintAssignable) FPGOnFinished OnMissionFinished;
	UPROPERTY(BlueprintAssignable) FPGOnStep OnStepChanged;

	/** Im Level-Blueprint: Hütte gegen zerstörte Version tauschen, Feuer einschalten usw. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void OnImpact(int32 Index, AActor* ImpactPoint);

	/** Im Level-Blueprint: Himmel, Nebel und Lichter für "Day", "Night" oder "Dawn" setzen. */
	UFUNCTION(BlueprintImplementableEvent, Category = "Mission")
	void OnTimeOfDay(FName Phase);

	// ---------- Abfragen und Steuerung ----------
	UFUNCTION(BlueprintPure, Category = "Mission")
	EPGMissionStep GetStep() const { return Step; }

	UFUNCTION(BlueprintPure, Category = "Mission")
	bool IsSpeaking() const { return CurrentLine != INDEX_NONE; }

	UFUNCTION(BlueprintPure, Category = "Mission")
	FText GetUIText(FName Key) const;

	UFUNCTION(BlueprintPure, Category = "Mission")
	FText GetObjective() const { return ObjectiveText; }

	UFUNCTION(BlueprintPure, Category = "Mission")
	AActor* GetObjectiveMarker() const { return ObjectiveMarker.Get(); }

	UFUNCTION(BlueprintPure, Category = "Mission")
	int32 GetSavedCount() const { return SavedCount; }

	UFUNCTION(BlueprintCallable, Category = "Mission")
	void SkipLine();

	UFUNCTION(BlueprintCallable, Category = "Mission")
	void MakeChoice(bool bTellDara);

	void NotifyInteracted(APGInteractableActor* Actor, APGCharacter* Player);
	void NotifyTrigger(FName TriggerId, APGCharacter* Player, bool bEntered);
	void NotifyDetected(FName CaptionKey);

private:
	void SetStep(EPGMissionStep NewStep);
	void SetObjective(FName Key, AActor* Marker);
	void ShowCaption(FName TitleKey, FName SubKey);
	void PlayGroup(const FString& Group, TFunction<void()> OnDone = nullptr);
	void PlayNextLine();
	void FinishLine();

	void StartAttack();
	void DoImpact();
	void StartWounded();
	void UpdateWoundedObjective();
	void StartEscape();
	void StartNight();
	void StartChoice();
	void FinishMission();
	void RespawnAt(AActor* Point);
	void SetTimeOfDay(FName Phase);

	APGCharacter* GetPlayer() const;

	EPGMissionStep Step = EPGMissionStep::Intro;
	FText ObjectiveText;
	TWeakObjectPtr<AActor> ObjectiveMarker;

	// Untertitel
	TArray<const FPGDialogueLine*> LineQueue;
	int32 CurrentLine = INDEX_NONE;
	TFunction<void()> QueueDone;
	FTimerHandle LineTimer;

	// Angriff und Verwundete
	int32 ImpactIndex = 0;
	FTimerHandle ImpactTimer;
	float WoundedTimeLeft = 0.f;
	int32 SavedCount = 0;
	bool bInCave = false;

	// Nacht
	bool bInListenZone = false;
	int32 ListenIndex = 0;
	TArray<const FPGDialogueLine*> ListenLines;

	bool bToldDara = false;
	bool bFailing = false;
	float StartTime = 0.f;
};
