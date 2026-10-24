#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "PGGuard.generated.h"

class USpotLightComponent;

/**
 * Wache in der Nacht bei den Hirtenhütten. Läuft ihre Punkte ab und sieht in
 * einem Kegel vor sich. Geduckt und im Dunkeln wird Rojhat erst aus der Nähe
 * bemerkt. Braucht kein NavMesh: die Wache läuft direkt von Punkt zu Punkt.
 */
UCLASS()
class PESHMERGAGAME_API APGGuard : public ACharacter
{
	GENERATED_BODY()

public:
	APGGuard();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Wache")
	TObjectPtr<USpotLightComponent> Flashlight;

	UPROPERTY(EditAnywhere, Category = "Wache", meta = (MakeEditWidget = true))
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, Category = "Wache")
	float WaitAtPoint = 3.f;

	UPROPERTY(EditAnywhere, Category = "Wache")
	float ViewDistance = 1600.f;

	/** Halber Öffnungswinkel des Sichtkegels in Grad. */
	UPROPERTY(EditAnywhere, Category = "Wache")
	float ViewHalfAngle = 50.f;

	/** Sichtweite wird geduckt mit diesem Faktor multipliziert. */
	UPROPERTY(EditAnywhere, Category = "Wache")
	float CrouchFactor = 0.45f;

	UPROPERTY(EditAnywhere, Category = "Wache")
	float SpotSeconds = 1.f;

	UFUNCTION(BlueprintCallable, Category = "Wache")
	void SetActive(bool bNewActive);

	UFUNCTION(BlueprintCallable, Category = "Wache")
	void ResetDetection();

	UFUNCTION(BlueprintPure, Category = "Wache")
	float GetSuspicion() const { return SpotSeconds > 0.f ? Suspicion / SpotSeconds : 0.f; }

protected:
	virtual void BeginPlay() override;

	bool CanSee(const APawn* Target) const;

	bool bActive = false;
	int32 PatrolIndex = 0;
	float WaitTime = 0.f;
	float Suspicion = 0.f;
	FTransform StartTransform;
};
