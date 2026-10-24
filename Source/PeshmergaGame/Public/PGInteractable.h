#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "PGInteractable.generated.h"

class APGCharacter;

UINTERFACE(MinimalAPI, Blueprintable)
class UPGInteractable : public UInterface
{
	GENERATED_BODY()
};

/** Alles, womit Rojhat mit Taste E interagieren kann. */
class PESHMERGAGAME_API IPGInteractable
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaktion")
	bool CanInteract(APGCharacter* User) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaktion")
	FText GetPrompt(APGCharacter* User) const;

	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaktion")
	void Interact(APGCharacter* User);
};
