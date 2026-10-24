#pragma once

#include "CoreMinimal.h"
#include "Engine/DataTable.h"
#include "PGTypes.generated.h"

class USoundBase;

/** Eine Dialogzeile, importiert aus RawData/Dialogue/DT_M01_Dialogue.csv. */
USTRUCT(BlueprintType)
struct FPGDialogueLine : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog")
	FName Speaker;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog")
	FText SpeakerDE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog")
	FText SpeakerCKB;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog")
	FText TextDE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog")
	FText TextCKB;

	/** Anzeigedauer in Sekunden, wenn keine Sprachaufnahme vorhanden ist. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog")
	float Duration = 3.f;

	/** Optionale Sprachaufnahme (Sorani). Bestimmt dann die Dauer. */
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Dialog")
	TSoftObjectPtr<USoundBase> Voice;
};

/** Oberflächentext in beiden Sprachen, importiert aus DT_M01_UIText.csv. */
USTRUCT(BlueprintType)
struct FPGUIText : public FTableRowBase
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FText TextDE;

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "Text")
	FText TextCKB;
};

UENUM(BlueprintType)
enum class EPGMissionStep : uint8
{
	Intro,
	Water,
	Tea,
	Radio,
	Attack,
	Cave,
	Wounded,
	Escape,
	Night,
	Listen,
	Return,
	Choice,
	Dawn,
	Finished,
	Failed
};

UENUM(BlueprintType)
enum class EPGInteractKind : uint8
{
	Talk,
	Water,
	Radio,
	Wounded
};

namespace PGText
{
	/** true, wenn die aktuelle Spielsprache Sorani (ckb) ist. */
	PESHMERGAGAME_API bool IsSorani();

	PESHMERGAGAME_API const FText& Pick(const FText& DE, const FText& CKB);
}
