#pragma once

#include "CoreMinimal.h"
#include "GameFramework/SaveGame.h"
#include "PGSaveGame.generated.h"

/** Fortschritt der Kampagne. Spätere Missionen lesen die Entscheidungen aus Mission 01. */
UCLASS()
class PESHMERGAGAME_API UPGSaveGame : public USaveGame
{
	GENERATED_BODY()

public:
	static constexpr const TCHAR* SlotName = TEXT("Kampagne");

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Kampagne")
	FString Language = TEXT("de");

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Mission 01")
	bool bM01Completed = false;

	/** Hat Rojhat Dara von Cemals Telefonat erzählt? */
	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Mission 01")
	bool bM01ToldDara = false;

	UPROPERTY(SaveGame, BlueprintReadWrite, Category = "Mission 01")
	int32 M01SavedWounded = 0;

	UFUNCTION(BlueprintCallable, Category = "Kampagne")
	static UPGSaveGame* LoadOrCreate();

	UFUNCTION(BlueprintCallable, Category = "Kampagne")
	static bool Store(UPGSaveGame* Save);
};
