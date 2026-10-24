#include "PGCharacter.h"
#include "PGInteractable.h"
#include "PGInteractableActor.h"
#include "PGMissionDirector.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputAction.h"
#include "InputMappingContext.h"
#include "InputModifiers.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"

APGCharacter::APGCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	GetCapsuleComponent()->InitCapsuleSize(34.f, 90.f);

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(GetCapsuleComponent());
	Camera->SetRelativeLocation(FVector(0.f, 0.f, 72.f));
	Camera->bUsePawnControlRotation = true;
	Camera->SetFieldOfView(80.f);

	CarryPoint = CreateDefaultSubobject<USceneComponent>(TEXT("CarryPoint"));
	CarryPoint->SetupAttachment(GetCapsuleComponent());
	CarryPoint->SetRelativeLocation(FVector(10.f, 30.f, 40.f));

	bUseControllerRotationYaw = true;
	UCharacterMovementComponent* MoveComp = GetCharacterMovement();
	MoveComp->MaxWalkSpeed = WalkSpeed;
	MoveComp->MaxWalkSpeedCrouched = 190.f;
	MoveComp->GetNavAgentPropertiesRef().bCanCrouch = true;
	MoveComp->bCanWalkOffLedgesWhenCrouching = true;
	MoveComp->JumpZVelocity = 380.f;
	MoveComp->AirControl = 0.15f;
	MoveComp->SetWalkableFloorAngle(50.f); // steile Bergpfade
	MoveComp->MaxStepHeight = 50.f;
}

void APGCharacter::PawnClientRestart()
{
	Super::PawnClientRestart();
	CreateRuntimeInput();

	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		if (UEnhancedInputLocalPlayerSubsystem* Subsystem = ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
		{
			Subsystem->ClearAllMappings();
			Subsystem->AddMappingContext(MappingContext, 0);
		}
	}
}

static UInputAction* MakeAction(UObject* Outer, const TCHAR* Name, EInputActionValueType Type)
{
	UInputAction* Action = NewObject<UInputAction>(Outer, Name);
	Action->ValueType = Type;
	return Action;
}

void APGCharacter::CreateRuntimeInput()
{
	if (MappingContext)
	{
		return;
	}

	MoveAction     = MakeAction(this, TEXT("IA_Move"), EInputActionValueType::Axis2D);
	LookAction     = MakeAction(this, TEXT("IA_Look"), EInputActionValueType::Axis2D);
	JumpAction     = MakeAction(this, TEXT("IA_Jump"), EInputActionValueType::Boolean);
	SprintAction   = MakeAction(this, TEXT("IA_Sprint"), EInputActionValueType::Boolean);
	CrouchAction   = MakeAction(this, TEXT("IA_Crouch"), EInputActionValueType::Boolean);
	InteractAction = MakeAction(this, TEXT("IA_Interact"), EInputActionValueType::Boolean);
	ChoiceTellAction   = MakeAction(this, TEXT("IA_ChoiceTell"), EInputActionValueType::Boolean);
	ChoiceSilentAction = MakeAction(this, TEXT("IA_ChoiceSilent"), EInputActionValueType::Boolean);

	MappingContext = NewObject<UInputMappingContext>(this, TEXT("IMC_Rojhat"));

	auto Swizzle = [this]() { return NewObject<UInputModifierSwizzleAxis>(this); };
	auto Negate = [this](bool bX, bool bY)
	{
		UInputModifierNegate* N = NewObject<UInputModifierNegate>(this);
		N->bX = bX; N->bY = bY; N->bZ = false;
		return N;
	};

	// WASD: W/S auf die Y-Achse (vorwärts), A/D auf die X-Achse (seitwärts)
	MappingContext->MapKey(MoveAction, EKeys::W).Modifiers.Add(Swizzle());
	{
		FEnhancedActionKeyMapping& S = MappingContext->MapKey(MoveAction, EKeys::S);
		S.Modifiers.Add(Swizzle());
		S.Modifiers.Add(Negate(true, true));
	}
	MappingContext->MapKey(MoveAction, EKeys::D);
	MappingContext->MapKey(MoveAction, EKeys::A).Modifiers.Add(Negate(true, false));
	MappingContext->MapKey(MoveAction, EKeys::Gamepad_Left2D);

	MappingContext->MapKey(LookAction, EKeys::Mouse2D).Modifiers.Add(Negate(false, true));
	MappingContext->MapKey(LookAction, EKeys::Gamepad_Right2D).Modifiers.Add(Negate(false, true));

	MappingContext->MapKey(JumpAction, EKeys::SpaceBar);
	MappingContext->MapKey(JumpAction, EKeys::Gamepad_FaceButton_Bottom);
	MappingContext->MapKey(SprintAction, EKeys::LeftShift);
	MappingContext->MapKey(SprintAction, EKeys::Gamepad_LeftThumbstick);
	MappingContext->MapKey(CrouchAction, EKeys::C);
	MappingContext->MapKey(CrouchAction, EKeys::Gamepad_FaceButton_Right);
	MappingContext->MapKey(InteractAction, EKeys::E);
	MappingContext->MapKey(InteractAction, EKeys::Gamepad_FaceButton_Left);
	MappingContext->MapKey(ChoiceTellAction, EKeys::One);
	MappingContext->MapKey(ChoiceTellAction, EKeys::Gamepad_DPad_Left);
	MappingContext->MapKey(ChoiceSilentAction, EKeys::Two);
	MappingContext->MapKey(ChoiceSilentAction, EKeys::Gamepad_DPad_Right);
}

void APGCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);
	CreateRuntimeInput();

	UEnhancedInputComponent* Input = CastChecked<UEnhancedInputComponent>(PlayerInputComponent);
	Input->BindAction(MoveAction, ETriggerEvent::Triggered, this, &APGCharacter::Move);
	Input->BindAction(LookAction, ETriggerEvent::Triggered, this, &APGCharacter::Look);
	Input->BindAction(JumpAction, ETriggerEvent::Started, this, &APGCharacter::JumpOrSkip);
	Input->BindAction(JumpAction, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
	Input->BindAction(SprintAction, ETriggerEvent::Started, this, &APGCharacter::SprintStart);
	Input->BindAction(SprintAction, ETriggerEvent::Completed, this, &APGCharacter::SprintStop);
	Input->BindAction(CrouchAction, ETriggerEvent::Started, this, &APGCharacter::ToggleCrouch);
	Input->BindAction(InteractAction, ETriggerEvent::Started, this, &APGCharacter::DoInteract);
	Input->BindAction(ChoiceTellAction, ETriggerEvent::Started, this, &APGCharacter::ChooseTell);
	Input->BindAction(ChoiceSilentAction, ETriggerEvent::Started, this, &APGCharacter::ChooseSilent);
}

void APGCharacter::ChooseTell()
{
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		Director->MakeChoice(true);
	}
}

void APGCharacter::ChooseSilent()
{
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		Director->MakeChoice(false);
	}
}

void APGCharacter::Move(const FInputActionValue& Value)
{
	if (bControlsLocked || !Controller)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Axis.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Axis.X);
}

void APGCharacter::Look(const FInputActionValue& Value)
{
	if (bControlsLocked)
	{
		return;
	}
	const FVector2D Axis = Value.Get<FVector2D>();
	AddControllerYawInput(Axis.X);
	AddControllerPitchInput(Axis.Y);
}

void APGCharacter::JumpOrSkip()
{
	// Leertaste überspringt laufende Untertitel, sonst wird gesprungen.
	if (APGMissionDirector* Director = APGMissionDirector::Get(this))
	{
		if (Director->IsSpeaking())
		{
			Director->SkipLine();
			return;
		}
	}
	if (!bControlsLocked && !IsCarrying())
	{
		Jump();
	}
}

void APGCharacter::SprintStart()
{
	bSprinting = true;
	UpdateSpeed();
}

void APGCharacter::SprintStop()
{
	bSprinting = false;
	UpdateSpeed();
}

void APGCharacter::ToggleCrouch()
{
	if (bControlsLocked)
	{
		return;
	}
	if (bIsCrouched)
	{
		UnCrouch();
	}
	else
	{
		Crouch();
	}
}

void APGCharacter::DoInteract()
{
	if (bControlsLocked)
	{
		return;
	}
	AActor* Target = FocusedActor.Get();
	if (Target && Target->Implements<UPGInteractable>() && IPGInteractable::Execute_CanInteract(Target, this))
	{
		IPGInteractable::Execute_Interact(Target, this);
		FocusedActor.Reset();
	}
}

void APGCharacter::UpdateSpeed()
{
	float Speed = (bSprinting && !IsCarrying()) ? SprintSpeed : WalkSpeed;
	if (IsCarrying())
	{
		Speed *= CarrySpeedFactor;
	}
	GetCharacterMovement()->MaxWalkSpeed = Speed;
}

void APGCharacter::StartCarry(APGInteractableActor* Wounded)
{
	if (!Wounded || CarriedActor)
	{
		return;
	}
	CarriedActor = Wounded;
	Wounded->bCarried = true;
	Wounded->SetActorEnableCollision(false);
	Wounded->AttachToComponent(CarryPoint, FAttachmentTransformRules::SnapToTargetNotIncludingScale);
	if (bIsCrouched)
	{
		UnCrouch();
	}
	UpdateSpeed();
}

APGInteractableActor* APGCharacter::StopCarry()
{
	APGInteractableActor* Dropped = CarriedActor;
	if (Dropped)
	{
		Dropped->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
		Dropped->SetActorEnableCollision(true);
		Dropped->bCarried = false;
		CarriedActor = nullptr;
		UpdateSpeed();
	}
	return Dropped;
}

FText APGCharacter::GetFocusPrompt() const
{
	AActor* Target = FocusedActor.Get();
	if (Target && Target->Implements<UPGInteractable>())
	{
		return IPGInteractable::Execute_GetPrompt(Target, const_cast<APGCharacter*>(this));
	}
	return FText::GetEmpty();
}

void APGCharacter::UpdateFocus()
{
	FocusedActor.Reset();
	if (bControlsLocked)
	{
		return;
	}

	const FVector Start = Camera->GetComponentLocation();
	const FVector End = Start + Camera->GetForwardVector() * InteractDistance;
	FCollisionQueryParams Params(SCENE_QUERY_STAT(PGInteract), false, this);
	if (CarriedActor)
	{
		Params.AddIgnoredActor(CarriedActor);
	}

	// Kugel statt Linie, damit man Figuren nicht pixelgenau treffen muss.
	FHitResult Hit;
	if (GetWorld()->SweepSingleByChannel(Hit, Start, End, FQuat::Identity, ECC_Visibility, FCollisionShape::MakeSphere(18.f), Params))
	{
		AActor* HitActor = Hit.GetActor();
		if (HitActor && HitActor->Implements<UPGInteractable>() && IPGInteractable::Execute_CanInteract(HitActor, this))
		{
			FocusedActor = HitActor;
		}
	}
}

void APGCharacter::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	UpdateFocus();

	// Kopfwippen abhängig von der Geschwindigkeit
	if (CameraBaseLocation.IsZero())
	{
		CameraBaseLocation = Camera->GetRelativeLocation();
	}
	const float Speed2D = GetVelocity().Size2D();
	if (Speed2D > 10.f && GetCharacterMovement()->IsMovingOnGround())
	{
		BobTime += DeltaSeconds * Speed2D / 55.f;
	}
	const float Bob = FMath::Sin(BobTime) * HeadBobAmount * FMath::Clamp(Speed2D / WalkSpeed, 0.f, 1.5f);
	Camera->SetRelativeLocation(CameraBaseLocation + FVector(0.f, 0.f, Bob));
}
