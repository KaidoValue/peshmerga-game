#include "PGTypes.h"
#include "Internationalization/Internationalization.h"
#include "Internationalization/Culture.h"

bool PGText::IsSorani()
{
	return FInternationalization::Get().GetCurrentLanguage()->GetTwoLetterISOLanguageName() == TEXT("ckb")
		|| FInternationalization::Get().GetCurrentLanguage()->GetName().StartsWith(TEXT("ckb"));
}

const FText& PGText::Pick(const FText& DE, const FText& CKB)
{
	return (IsSorani() && !CKB.IsEmpty()) ? CKB : DE;
}
