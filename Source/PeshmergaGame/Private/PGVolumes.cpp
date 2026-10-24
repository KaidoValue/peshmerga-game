#include "PGVolumes.h"
#include "PGCharacter.h"
#include "PGMissionDirector.h"
#include "Components/BoxComponent.h"

static UBoxComponent* MakeTriggerBox(AActor* Owner, const FColor& Color)
{
	UBoxComponent* Box = Owner->CreateDefaultSubobject<UBoxComponent>(TEXT("Box"));
	Box->SetBoxExtent(FVector(300.f, 300.f, 200.f));
	Box->SetCollisionProfileName(TEXT("Trigger"));
	Box->ShapeColor = Color;
	Box->SetGenerateOverlapEvents(true);
	return Box;
}

APGMissionTrigger::APGMissionTrigger()
{
	Box = MakeTriggerBox(this, FColor(255, 170, 40));
	SetRootComponent(Box);
}

void APGMissionTrigger::BeginPlay()
{
	Super::BeginPlay();
	Box->OnComponentBeginOverlap.AddDynamic(this, &APGMissionTrigger::HandleBegin);
	Box->OnComponentEndOverlap.AddDynamic(this, &APGMissionTrigger::HandleEnd);
}

void APGMissionTrigger::HandleBegin(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (APGCharacter* Player = Cast<APGCharacter>(Other))
	{
		if (APGMissionDirector* Director = APGMissionDirector::Get(this))
		{
			Director->NotifyTrigger(TriggerId, Player, true);
		}
	}
}

void APGMissionTrigger::HandleEnd(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (APGCharacter* Player = Cast<APGCharacter>(Other))
	{
		if (APGMissionDirector* Director = APGMissionDirector::Get(this))
		{
			Director->NotifyTrigger(TriggerId, Player, false);
		}
	}
}

APGShelterVolume::APGShelterVolume()
{
	Box = MakeTriggerBox(this, FColor(60, 200, 255));
	SetRootComponent(Box);
}

void APGShelterVolume::BeginPlay()
{
	Super::BeginPlay();
	Box->OnComponentBeginOverlap.AddDynamic(this, &APGShelterVolume::HandleBegin);
	Box->OnComponentEndOverlap.AddDynamic(this, &APGShelterVolume::HandleEnd);
}

void APGShelterVolume::HandleBegin(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32, bool, const FHitResult&)
{
	if (APGCharacter* Player = Cast<APGCharacter>(Other))
	{
		Player->EnterShelter();
	}
}

void APGShelterVolume::HandleEnd(UPrimitiveComponent*, AActor* Other, UPrimitiveComponent*, int32)
{
	if (APGCharacter* Player = Cast<APGCharacter>(Other))
	{
		Player->LeaveShelter();
	}
}
