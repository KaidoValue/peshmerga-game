#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PGReconDrone.generated.h"

class UStaticMeshComponent;
class USpotLightComponent;
class UDecalComponent;
class UAudioComponent;

/**
 * Aufklärungsdrohne mit Wärmebildkamera. Ihr Blickfeld ist ein Kreis auf dem
 * Boden. Wer länger als DetectSeconds darin steht und nicht unter einem
 * Felsdach (APGShelterVolume) ist, wird entdeckt. Die Drohne wird nie aus der
 * Nähe gezeigt.
 */
UCLASS()
class PESHMERGAGAME_API APGReconDrone : public AActor
{
	GENERATED_BODY()

public:
	APGReconDrone();

	virtual void Tick(float DeltaSeconds) override;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drohne")
	TObjectPtr<UStaticMeshComponent> Body;

	/** Roter Lichtkegel als Hinweis auf das Blickfeld. */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drohne")
	TObjectPtr<USpotLightComponent> ThermalCone;

	/** Roter Kreis auf dem Gelände (Material mit Emissive zuweisen). */
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drohne")
	TObjectPtr<UDecalComponent> Footprint;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Drohne")
	TObjectPtr<UAudioComponent> Hum;

	/** Patrouillenpunkte relativ zur Drohne (im Editor als Griffe verschiebbar). */
	UPROPERTY(EditAnywhere, Category = "Drohne", meta = (MakeEditWidget = true))
	TArray<FVector> PatrolPoints;

	UPROPERTY(EditAnywhere, Category = "Drohne")
	float Speed = 900.f;

	/** Flughöhe über dem Gelände in cm. */
	UPROPERTY(EditAnywhere, Category = "Drohne")
	float Altitude = 9000.f;

	/** Radius des Blickfelds auf dem Boden in cm. */
	UPROPERTY(EditAnywhere, Category = "Drohne")
	float FootprintRadius = 1400.f;

	UPROPERTY(EditAnywhere, Category = "Drohne")
	float DetectSeconds = 1.2f;

	/** Anteil, mit dem die Drohne dem Spieler folgt, sobald er sichtbar ist (0..1). */
	UPROPERTY(EditAnywhere, Category = "Drohne", meta = (ClampMin = "0", ClampMax = "1"))
	float HuntFactor = 0.35f;

	UFUNCTION(BlueprintCallable, Category = "Drohne")
	void SetActive(bool bNewActive);

	UFUNCTION(BlueprintCallable, Category = "Drohne")
	void ResetDetection() { Detection = 0.f; }

	/** 0..1, für eine Warnanzeige im HUD. */
	UFUNCTION(BlueprintPure, Category = "Drohne")
	float GetDetection() const { return DetectSeconds > 0.f ? Detection / DetectSeconds : 0.f; }

protected:
	virtual void BeginPlay() override;

	FVector GroundBelow(const FVector& Point) const;

	bool bActive = false;
	int32 PatrolIndex = 0;
	float Detection = 0.f;
	FVector Origin = FVector::ZeroVector;
	FTransform OriginTransform;
	FVector GroundPoint = FVector::ZeroVector;
};
