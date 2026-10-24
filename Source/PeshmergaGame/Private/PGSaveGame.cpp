#include "PGSaveGame.h"
#include "Kismet/GameplayStatics.h"

UPGSaveGame* UPGSaveGame::LoadOrCreate()
{
	if (UGameplayStatics::DoesSaveGameExist(SlotName, 0))
	{
		if (UPGSaveGame* Loaded = Cast<UPGSaveGame>(UGameplayStatics::LoadGameFromSlot(SlotName, 0)))
		{
			return Loaded;
		}
	}
	return Cast<UPGSaveGame>(UGameplayStatics::CreateSaveGameObject(UPGSaveGame::StaticClass()));
}

bool UPGSaveGame::Store(UPGSaveGame* Save)
{
	return Save && UGameplayStatics::SaveGameToSlot(Save, SlotName, 0);
}
