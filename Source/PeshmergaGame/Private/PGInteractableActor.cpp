#include "PGInteractableActor.h"
#include "PGCharacter.h"
#include "PGMissionDirector.h"
#include "Components/StaticMeshComponent.h"

APGInteractableActor::APGInteractableActor()
{
	PrimaryActorTick.bCanEverTick = false;

	Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
	SetRootComponent(Root);

	Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
	Mesh->SetupAttachment(Root);
	Mesh->SetCollisionProfileName(TEXT("PGInteract"));
}

bool APGInteractableActor::CanInteract_Implementation(APGCharacter* User) const
{
	const APGMissionDirector* Director = APGMissionDirector::Get(this);
	if (!Director || !User || !ActiveSteps.Contains(Director->GetStep()) || bRescued)
	{
		return false;
	}
	if (Kind == EPGInteractKind::Wounded)
	{
		return !bCarried && !User->IsCarrying();
	}
	return !Director->IsSpeaking();
}

FText APGInteractableActor::GetPrompt_Implementation(APGCharacter* User) const
{
	const APGMissionDirector* Director = APGMissionDirector::Get(this);
	if (!Director)
	{
		return FText::GetEmpty();
	}
	switch (Kind)
	{
	case EPGInteractKind::Water:   return Director->GetUIText(TEXT("pWater"));
	case EPGInteractKind::Wounded: return Director->GetUIText(TEXT("pCarry"));
	default:                       return Director->GetUIText(TEXT("pTalk"));
	}
}

void APGInteractableActor::Interact_Implementation(APGCharacter* User)
{
	if (Kind == EPGInteractKind::Wounded)
	{
		User->StartCarry(this);
	}
	OnInteracted.Broadcast(this, User);
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		Director->NotifyInteracted(this, User);
	}
}
