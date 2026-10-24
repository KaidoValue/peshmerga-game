#include "PGGuard.h"
#include "PGCharacter.h"
#include "PGMissionDirector.h"
#include "AIController.h"
#include "Components/CapsuleComponent.h"
#include "Components/SpotLightComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"

APGGuard::APGGuard()
{
	PrimaryActorTick.bCanEverTick = true;
	AutoPossessAI = EAutoPossessAI::PlacedInWorldOrSpawned;
	AIControllerClass = AAIController::StaticClass();

	GetCharacterMovement()->MaxWalkSpeed = 140.f;
	GetCharacterMovement()->bOrientRotationToMovement = true;
	GetCharacterMovement()->RotationRate = FRotator(0.f, 180.f, 0.f);
	bUseControllerRotationYaw = false;

	Flashlight = CreateDefaultSubobject<USpotLightComponent>(TEXT("Flashlight"));
	Flashlight->SetupAttachment(GetCapsuleComponent());
	Flashlight->SetRelativeLocation(FVector(20.f, 10.f, 40.f));
	Flashlight->SetRelativeRotation(FRotator(-12.f, 0.f, 0.f));
	Flashlight->SetIntensityUnits(ELightUnits::Candelas);
	Flashlight->SetIntensity(900.f);
	Flashlight->SetAttenuationRadius(2200.f);
	Flashlight->SetOuterConeAngle(24.f);
	Flashlight->SetInnerConeAngle(14.f);
	Flashlight->SetLightColor(FLinearColor(1.f, 0.9f, 0.75f));
}

void APGGuard::BeginPlay()
{
	Super::BeginPlay();
	StartTransform = GetActorTransform();
	SetActive(bActive);
}

void APGGuard::SetActive(bool bNewActive)
{
	bActive = bNewActive;
	SetActorHiddenInGame(!bActive);
	Flashlight->SetVisibility(bActive);
	ResetDetection();
}

void APGGuard::ResetDetection()
{
	Suspicion = 0.f;
}

bool APGGuard::CanSee(const APawn* Target) const
{
	if (!Target)
	{
		return false;
	}
	const APGCharacter* Player = Cast<APGCharacter>(Target);
	const float Range = ViewDistance * ((Player && Player->bIsCrouched) ? CrouchFactor : 1.f);

	const FVector Eye = GetActorLocation() + FVector(0.f, 0.f, BaseEyeHeight);
	const FVector ToTarget = Target->GetActorLocation() - Eye;
	const float Dist = ToTarget.Size();
	if (Dist > Range)
	{
		return false;
	}
	// Ganz nah hört die Wache dich immer.
	if (Dist > 180.f)
	{
		const float CosAngle = FVector::DotProduct(GetActorForwardVector(), ToTarget / Dist);
		if (CosAngle < FMath::Cos(FMath::DegreesToRadians(ViewHalfAngle)))
		{
			return false;
		}
	}
	FHitResult Hit;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PGGuardSight), false, this);
	Params.AddIgnoredActor(Target);
	return !GetWorld()->LineTraceSingleByChannel(Hit, Eye, Target->GetActorLocation(), ECC_Visibility, Params);
}

void APGGuard::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	if (!bActive)
	{
		return;
	}

	if (PatrolPoints.Num() > 0)
	{
		const FVector Target = StartTransform.TransformPosition(PatrolPoints[PatrolIndex]);
		FVector Dir = Target - GetActorLocation();
		Dir.Z = 0.f;
		if (Dir.Size() < 60.f)
		{
			WaitTime += DeltaSeconds;
			if (WaitTime >= WaitAtPoint)
			{
				WaitTime = 0.f;
				PatrolIndex = (PatrolIndex + 1) % PatrolPoints.Num();
			}
		}
		else
		{
			AddMovementInput(Dir.GetSafeNormal(), 1.f);
		}
	}

	APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0);
	if (CanSee(Player))
	{
		Suspicion += DeltaSeconds;
		if (Suspicion >= SpotSeconds)
		{
			Suspicion = 0.f;
			if (APGMissionDirector* Director = APGMissionDirector::Get(this))
			{
				Director->NotifyDetected(TEXT("capSeen"));
			}
		}
	}
	else
	{
		Suspicion = FMath::Max(0.f, Suspicion - DeltaSeconds * 0.6f);
	}
}
