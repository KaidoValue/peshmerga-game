#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "InputActionValue.h"
#include "PGCharacter.generated.h"

class UCameraComponent;
class USceneComponent;
class UInputAction;
class UInputMappingContext;
class APGInteractableActor;

/**
 * Rojhat in der Ich-Perspektive. Keine Waffe in Mission 01.
 * Die Eingabe (Enhanced Input) wird zur Laufzeit in C++ erzeugt, damit das
 * Projekt ohne Input-Assets sofort spielbar ist.
 */
UCLASS()
class PESHMERGAGAME_API APGCharacter : public ACharacter
{
	GENERATED_BODY()

public:
	APGCharacter();

	virtual void Tick(float DeltaSeconds) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;
	virtual void PawnClientRestart() override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Kamera")
	TObjectPtr<UCameraComponent> Camera;

	/** Hier hängt ein getragener Verwundeter. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Tragen")
	TObjectPtr<USceneComponent> CarryPoint;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bewegung")
	float WalkSpeed = 380.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bewegung")
	float SprintSpeed = 620.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Bewegung")
	float CarrySpeedFactor = 0.55f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interaktion")
	float InteractDistance = 260.f;

	/** Leichtes Kopfwippen beim Gehen, in Zentimetern. */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Kamera")
	float HeadBobAmount = 1.6f;

	UFUNCTION(BlueprintCallable, Category = "Tragen")
	void StartCarry(APGInteractableActor* Wounded);

	/** Gibt den getragenen Verwundeten zurück (oder nullptr) und lässt ihn los. */
	UFUNCTION(BlueprintCallable, Category = "Tragen")
	APGInteractableActor* StopCarry();

	UFUNCTION(BlueprintPure, Category = "Tragen")
	bool IsCarrying() const { return CarriedActor != nullptr; }

	UFUNCTION(BlueprintPure, Category = "Interaktion")
	AActor* GetFocusedActor() const { return FocusedActor.Get(); }

	UFUNCTION(BlueprintPure, Category = "Interaktion")
	FText GetFocusPrompt() const;

	/** Unter einem Felsdach versteckt (vor der Wärmebild-Drohne). */
	UFUNCTION(BlueprintPure, Category = "Tarnung")
	bool IsSheltered() const { return ShelterCount > 0; }

	void EnterShelter() { ++ShelterCount; }
	void LeaveShelter() { ShelterCount = FMath::Max(0, ShelterCount - 1); }

	/** Steuerung sperren (Zwischensequenzen, Entscheidung). */
	UFUNCTION(BlueprintCallable, Category = "Steuerung")
	void SetControlsLocked(bool bLocked) { bControlsLocked = bLocked; }

protected:
	void Move(const FInputActionValue& Value);
	void Look(const FInputActionValue& Value);
	void JumpOrSkip();
	void SprintStart();
	void SprintStop();
	void ToggleCrouch();
	void DoInteract();
	void ChooseTell();
	void ChooseSilent();

	void CreateRuntimeInput();
	void UpdateFocus();
	void UpdateSpeed();

	UPROPERTY(Transient) TObjectPtr<UInputMappingContext> MappingContext;
	UPROPERTY(Transient) TObjectPtr<UInputAction> MoveAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> LookAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> JumpAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> SprintAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> CrouchAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> InteractAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ChoiceTellAction;
	UPROPERTY(Transient) TObjectPtr<UInputAction> ChoiceSilentAction;

	UPROPERTY(Transient) TObjectPtr<APGInteractableActor> CarriedActor;

	TWeakObjectPtr<AActor> FocusedActor;
	int32 ShelterCount = 0;
	bool bSprinting = false;
	bool bControlsLocked = false;
	float BobTime = 0.f;
	FVector CameraBaseLocation = FVector::ZeroVector;
};
