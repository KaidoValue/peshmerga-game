#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PGVolumes.generated.h"

class UBoxComponent;

/** Meldet dem Missionsablauf, wenn Rojhat einen Bereich betritt: "Cave", "Hut" oder "Listen". */
UCLASS()
class PESHMERGAGAME_API APGMissionTrigger : public AActor
{
	GENERATED_BODY()

public:
	APGMissionTrigger();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Trigger")
	TObjectPtr<UBoxComponent> Box;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Trigger")
	FName TriggerId = TEXT("Cave");

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBegin(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void HandleEnd(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);
};

/** Felsdach oder Überhang: darunter sieht die Wärmebild-Drohne Rojhat nicht. */
UCLASS()
class PESHMERGAGAME_API APGShelterVolume : public AActor
{
	GENERATED_BODY()

public:
	APGShelterVolume();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Versteck")
	TObjectPtr<UBoxComponent> Box;

protected:
	virtual void BeginPlay() override;

	UFUNCTION()
	void HandleBegin(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex, bool bFromSweep, const FHitResult& Sweep);

	UFUNCTION()
	void HandleEnd(UPrimitiveComponent* Comp, AActor* Other, UPrimitiveComponent* OtherComp, int32 BodyIndex);
};
