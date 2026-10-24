#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PGInteractable.h"
#include "PGTypes.h"
#include "PGInteractableActor.generated.h"

class UStaticMeshComponent;
class USceneComponent;

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FPGOnInteracted, APGInteractableActor*, Actor, APGCharacter*, Instigator);

/**
 * Quelle, Person (Dara, Azad), Funkgerät oder Verwundeter.
 * Für Figuren den Mesh durch einen MetaHuman-Blueprint als Child-Actor ersetzen.
 */
UCLASS()
class PESHMERGAGAME_API APGInteractableActor : public AActor, public IPGInteractable
{
	GENERATED_BODY()

public:
	APGInteractableActor();

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaktion")
	TObjectPtr<USceneComponent> Root;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Interaktion")
	TObjectPtr<UStaticMeshComponent> Mesh;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaktion")
	EPGInteractKind Kind = EPGInteractKind::Talk;

	/** Interaktion ist nur in diesen Missionsschritten möglich (Dara z. B. bei Tee und Rückkehr). */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Interaktion")
	TArray<EPGMissionStep> ActiveSteps;

	/** Für Verwundete: wird gerade getragen oder ist schon in der Höhle. */
	UPROPERTY(BlueprintReadOnly, Category = "Interaktion")
	bool bCarried = false;

	UPROPERTY(BlueprintReadOnly, Category = "Interaktion")
	bool bRescued = false;

	UPROPERTY(BlueprintAssignable, Category = "Interaktion")
	FPGOnInteracted OnInteracted;

	virtual bool CanInteract_Implementation(APGCharacter* Instigator) const override;
	virtual FText GetPrompt_Implementation(APGCharacter* Instigator) const override;
	virtual void Interact_Implementation(APGCharacter* Instigator) override;
};
