#include "PGReconDrone.h"
#include "PGCharacter.h"
#include "PGMissionDirector.h"
#include "Components/AudioComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

APGReconDrone::APGReconDrone()
{
	PrimaryActorTick.bCanEverTick = true;

	Body = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Body"));
	SetRootComponent(Body);
	Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	Body->SetCastShadow(false);

	ThermalCone = CreateDefaultSubobject<USpotLightComponent>(TEXT("ThermalCone"));
	ThermalCone->SetupAttachment(Body);
	ThermalCone->SetRelativeRotation(FRotator(-90.f, 0.f, 0.f));
	ThermalCone->SetLightColor(FLinearColor(1.f, 0.12f, 0.08f));
	ThermalCone->SetIntensityUnits(ELightUnits::Candelas);
	ThermalCone->SetIntensity(1800.f);
	ThermalCone->SetAttenuationRadius(14000.f);
	ThermalCone->SetOuterConeAngle(9.f);
	ThermalCone->SetInnerConeAngle(6.f);
	ThermalCone->SetCastShadows(false);

	Footprint = CreateDefaultSubobject<UDecalComponent>(TEXT("Footprint"));
	Footprint->SetupAttachment(Body);
	Footprint->SetUsingAbsoluteLocation(true);
	Footprint->SetUsingAbsoluteRotation(true);
	Footprint->SetWorldRotation(FRotator(-90.f, 0.f, 0.f));

	Hum = CreateDefaultSubobject<UAudioComponent>(TEXT("Hum"));
	Hum->SetupAttachment(Body);
	Hum->bAutoActivate = false;
}

void APGReconDrone::BeginPlay()
{
	Super::BeginPlay();
	Origin = GetActorLocation();
	OriginTransform = GetActorTransform();
	Footprint->DecalSize = FVector(400.f, FootprintRadius, FootprintRadius);
	SetActive(bActive);
}

void APGReconDrone::SetActive(bool bNewActive)
{
	bActive = bNewActive;
	Detection = 0.f;
	SetActorHiddenInGame(!bActive);
	SetActorTickEnabled(bActive);
	if (bActive)
	{
		Hum->Play();
	}
	else
	{
		Hum->Stop();
	}
}

FVector APGReconDrone::GroundBelow(const FVector& Point) const
{
	FHitResult Hit;
	const FVector Start(Point.X, Point.Y, Point.Z + 50000.f);
	const FVector End(Point.X, Point.Y, Point.Z - 100000.f);
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PGDroneGround), false, this);
	if (APawn* Player = UGameplayStatics::GetPlayerPawn(this, 0))
	{
		Params.AddIgnoredActor(Player);
	}
	if (GetWorld()->LineTraceSingleByChannel(Hit, Start, End, ECC_Visibility, Params))
	{
		return Hit.ImpactPoint;
	}
	return FVector(Point.X, Point.Y, Origin.Z - Altitude);
}

void APGReconDrone::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);

	APGCharacter* Player = Cast<APGCharacter>(UGameplayStatics::GetPlayerPawn(this, 0));

	// Patrouille in der Ebene, Höhe folgt dem Gelände.
	FVector Target2D = GetActorLocation();
	if (PatrolPoints.Num() > 0)
	{
		const FVector WorldPoint = OriginTransform.TransformPosition(PatrolPoints[PatrolIndex]);
		Target2D = WorldPoint;
		if (FVector::Dist2D(GetActorLocation(), WorldPoint) < 200.f)
		{
			PatrolIndex = (PatrolIndex + 1) % PatrolPoints.Num();
		}
	}

	const bool bSeesPlayer = Player && !Player->IsSheltered()
		&& FVector::Dist2D(Player->GetActorLocation(), GroundPoint) < FootprintRadius;
	if (bSeesPlayer)
	{
		Target2D = FMath::Lerp(Target2D, Player->GetActorLocation(), HuntFactor);
	}

	FVector Dir = Target2D - GetActorLocation();
	Dir.Z = 0.f;
	const FVector Delta = Dir.GetSafeNormal() * FMath::Min(Dir.Size(), Speed * DeltaSeconds);
	GroundPoint = GroundBelow(GetActorLocation() + Delta);
	const float TargetZ = GroundPoint.Z + Altitude;
	FVector NewLoc = GetActorLocation() + Delta;
	NewLoc.Z = FMath::FInterpTo(GetActorLocation().Z, TargetZ, DeltaSeconds, 0.6f);
	SetActorLocation(NewLoc);
	Footprint->SetWorldLocation(GroundPoint);

	// Entdeckung
	if (bSeesPlayer)
	{
		Detection += DeltaSeconds;
		if (Detection >= DetectSeconds)
		{
			Detection = 0.f;
			if (APGMissionDirector* Director = APGMissionDirector::Get(this))
			{
				Director->NotifyDetected(TEXT("capCaught"));
			}
		}
	}
	else
	{
		Detection = FMath::Max(0.f, Detection - DeltaSeconds * 0.5f);
	}
}
