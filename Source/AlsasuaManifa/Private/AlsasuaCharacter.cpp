#include "AlsasuaCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "AlsasuaAttributeSet.h"
#include "CharacterTrajectoryComponent.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "Engine/LocalPlayer.h"
#include "AbilitySystemGlobals.h"
#include "GameplayEffectTypes.h"

AAlsasuaCharacter::AAlsasuaCharacter()
{
	PrimaryActorTick.bCanEverTick = true;

	CameraBoom = CreateDefaultSubobject<USpringArmComponent>(TEXT("CameraBoom"));
	CameraBoom->SetupAttachment(RootComponent);
	CameraBoom->TargetArmLength = 400.0f;
	CameraBoom->bUsePawnControlRotation = true;
	CameraBoom->bEnableCameraLag = true;
	CameraBoom->CameraLagSpeed = 12.f;
	CameraBoom->SocketOffset = FVector(0.f, 40.f, 60.f);

	FollowCamera = CreateDefaultSubobject<UCameraComponent>(TEXT("FollowCamera"));
	FollowCamera->SetupAttachment(CameraBoom, USpringArmComponent::SocketName);
	FollowCamera->bUsePawnControlRotation = false;

	AbilitySystemComponent = CreateDefaultSubobject<UAlsasuaAbilitySystemComponent>(TEXT("AbilitySystemComponent"));
	AttributeSet = CreateDefaultSubobject<UAlsasuaAttributeSet>(TEXT("AttributeSet"));

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->bOrientRotationToMovement = true;
		Mv->RotationRate = FRotator(0.f, 540.f, 0.f);
		Mv->MaxWalkSpeed = VelCaminar;
		Mv->MaxAcceleration = 1500.f;
		Mv->BrakingDecelerationWalking = 2000.f;
		Mv->MinAnalogWalkSpeed = 20.f;
		Mv->NavAgentProps.bCanCrouch = true;
		Mv->JumpZVelocity = 500.f;
		Mv->AirControl = 0.35f;
	}
}

void AAlsasuaCharacter::BeginPlay()
{
	Super::BeginPlay();
	if (USkeletalMeshComponent* CharacterMesh = GetMesh(); CharacterMesh && !CharacterMesh->GetSkeletalMeshAsset())
	{
		if (USkeletalMesh* Body = LoadObject<USkeletalMesh>(nullptr, TEXT("/Game/Man/Demo/Mesh/SK_Mannequin.SK_Mannequin")))
		{
			CharacterMesh->SetSkeletalMesh(Body);
			CharacterMesh->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));

			// AnimBP real con idle/walk/run integrado (paquete externo Man + Shrubs).
			if (UClass* AnimBP = LoadObject<UClass>(nullptr, TEXT("/Game/GV_FreeShrubsPack/Demo/Mannequin/Animations/ABP_Manny.ABP_Manny_C")))
				CharacterMesh->SetAnimInstanceClass(AnimBP);
		}
	}
	InitializeGAS();
	AsegurarInputRuntime();
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			if (ContextoMapeo) Sub->AddMappingContext(ContextoMapeo, 0);
}

void AAlsasuaCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	if (bTrepando)
	{
		TrepaT += DeltaTime;
		const float a = FMath::Clamp(TrepaT / TrepaDur, 0.f, 1.f);
		SetActorLocation(FMath::Lerp(TrepaInicio, TrepaFin, FMath::SmoothStep(0.f, 1.f, a)));
		if (a >= 1.f)
		{
			bTrepando = false;
			if (UCharacterMovementComponent* Mv = GetCharacterMovement()) Mv->SetMovementMode(MOVE_Walking);
		}
	}

	if (!CameraBoom || !FollowCamera) return;

	const bool bMov = GetVelocity().SizeSquared2D() > 100.f * 100.f;
	float kBrazo, kFOV;
	if (bApuntando) { kBrazo = BrazoApuntar; kFOV = FOVApuntar; }
	else if (bCorriendo && bMov) { kBrazo = BrazoCorrer; kFOV = FOVCorrer; }
	else { kBrazo = BrazoCadera; kFOV = FOVCadera; }

	CameraBoom->TargetArmLength = FMath::FInterpTo(CameraBoom->TargetArmLength, kBrazo, DeltaTime, 8.f);
	FollowCamera->FieldOfView = FMath::FInterpTo(FollowCamera->FieldOfView, kFOV, DeltaTime, 8.f);
}

void AAlsasuaCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AsegurarInputRuntime();

	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Mover)  EIC->BindAction(IA_Mover, ETriggerEvent::Triggered, this, &AAlsasuaCharacter::EntradaMover);
		if (IA_Mirar)  EIC->BindAction(IA_Mirar, ETriggerEvent::Triggered, this, &AAlsasuaCharacter::EntradaMirar);
		if (IA_Saltar)
		{
			EIC->BindAction(IA_Saltar, ETriggerEvent::Started,   this, &AAlsasuaCharacter::SaltarOTrepar);
			EIC->BindAction(IA_Saltar, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (IA_Correr)
		{
			EIC->BindAction(IA_Correr, ETriggerEvent::Started,   this, &AAlsasuaCharacter::CorrerInicio);
			EIC->BindAction(IA_Correr, ETriggerEvent::Completed, this, &AAlsasuaCharacter::CorrerFin);
		}
		if (IA_Agacharse) EIC->BindAction(IA_Agacharse, ETriggerEvent::Started, this, &AAlsasuaCharacter::AgacharseToggle);
		if (IA_Apuntar)
		{
			EIC->BindAction(IA_Apuntar, ETriggerEvent::Started,   this, &AAlsasuaCharacter::ApuntarInicio);
			EIC->BindAction(IA_Apuntar, ETriggerEvent::Completed, this, &AAlsasuaCharacter::ApuntarFin);
		}
	}

	// Legacy fallback: only bind if Enhanced Input actions are not set.
	if (!IA_Mover)
	{
		PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AAlsasuaCharacter::MoveForward);
		PlayerInputComponent->BindAxis(TEXT("MoveRight"),   this, &AAlsasuaCharacter::MoveRight);
		PlayerInputComponent->BindAxis(TEXT("Turn"),        this, &AAlsasuaCharacter::TurnAt);
		PlayerInputComponent->BindAxis(TEXT("LookUp"),      this, &AAlsasuaCharacter::LookUpAt);
		PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed,  this, &ACharacter::Jump);
		PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
	}
}

// ── GAS ─────────────────────────────────────────────────────────────────────

void AAlsasuaCharacter::InitializeGAS()
{
	if (AbilitySystemComponent)
	{
		AbilitySystemComponent->InitAbilityActorInfo(this, this);
		UE_LOG(LogAlsasuaGAS, Log, TEXT("GAS inicializado: %s"), *GetName());
	}
}

UAbilitySystemComponent* AAlsasuaCharacter::GetAbilitySystemComponent() const
{
	return Cast<UAbilitySystemComponent>(AbilitySystemComponent);
}

float AAlsasuaCharacter::GetHealth() const
{
	return AttributeSet ? AttributeSet->GetHealth() : 0.f;
}

float AAlsasuaCharacter::GetStamina() const
{
	return AttributeSet ? AttributeSet->GetStamina() : 0.f;
}

float AAlsasuaCharacter::GetPopularSupport() const
{
	return AttributeSet ? AttributeSet->GetPopularSupport() : 0.f;
}

// ── IDamageable bridge a GAS ────────────────────────────────────────────────

int32 AAlsasuaCharacter::GetVida() const
{
	return FMath::RoundToInt(GetHealth());
}

int32 AAlsasuaCharacter::GetVidaMax() const
{
	return AttributeSet ? FMath::RoundToInt(AttributeSet->GetMaxHealth()) : 100;
}

bool AAlsasuaCharacter::EstaMuerto() const
{
	return GetHealth() <= 0.f;
}

void AAlsasuaCharacter::RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo)
{
	if (!AbilitySystemComponent || !AttributeSet) return;

	float DanoReal = static_cast<float>(Cantidad);

	// Explosiones ignoran armadura y hacen daño doble.
	if (Tipo == ETipoDano::Explosion)
	{
		DanoReal *= 2.0f;
	}
	// Balas hacen daño base, otras formas reducen un 30%.
	else if (Tipo != ETipoDano::Bala)
	{
		DanoReal *= 0.7f;
	}

	AbilitySystemComponent->ApplyModToAttribute(AttributeSet->GetHealthAttribute(),
		EGameplayModOp::Additive, -DanoReal);

	if (GetHealth() <= 0.f)
	{
		AbilitySystemComponent->ApplyModToAttribute(AttributeSet->GetWantedLevelAttribute(),
			EGameplayModOp::Additive, 1.f);
	}
}

void AAlsasuaCharacter::Curar(int32 Cantidad)
{
	if (!AbilitySystemComponent || !AttributeSet) return;

	AbilitySystemComponent->ApplyModToAttribute(AttributeSet->GetHealthAttribute(),
		EGameplayModOp::Additive, static_cast<float>(Cantidad));
}

// ── Locomoción ──────────────────────────────────────────────────────────────

float AAlsasuaCharacter::GetSpeed2D() const
{
	return GetCharacterMovement() ? GetCharacterMovement()->Velocity.Size2D() : 0.f;
}

float AAlsasuaCharacter::GetMovementDirection() const
{
	if (!GetCharacterMovement()) return 0.f;
	const FVector Velocity = GetCharacterMovement()->Velocity;
	if (Velocity.SizeSquared2D() < 1.f) return 0.f;
	const FVector Fwd = GetActorForwardVector();
	const FVector Dir = Velocity.GetSafeNormal2D();
	return FMath::RadiansToDegrees(FMath::Atan2(FVector::CrossProduct(Fwd, Dir).Z, FVector::DotProduct(Fwd, Dir)));
}

EMovementGait AAlsasuaCharacter::GetMovementGait() const
{
	if (!GetCharacterMovement()) return EMovementGait::Idle;
	const float Speed = GetCharacterMovement()->Velocity.Size2D();
	if (Speed < 10.f) return EMovementGait::Idle;
	if (Speed < 200.f) return EMovementGait::Walking;
	if (Speed < 500.f) return EMovementGait::Running;
	return EMovementGait::Sprinting;
}

bool AAlsasuaCharacter::IsRunning() const
{
	const EMovementGait G = GetMovementGait();
	return G == EMovementGait::Running || G == EMovementGait::Sprinting;
}

bool AAlsasuaCharacter::IsCrouchingState() const
{
	return GetCharacterMovement() && GetCharacterMovement()->IsCrouching();
}

bool AAlsasuaCharacter::CanVault() const
{
	return true;
}

float AAlsasuaCharacter::GetAimOffsetYaw() const
{
	return GetBaseAimRotation().Yaw - GetActorRotation().Yaw;
}

float AAlsasuaCharacter::GetAimOffsetPitch() const
{
	return GetBaseAimRotation().Pitch;
}

float AAlsasuaCharacter::GetAimYawRate() const
{
	return (GetWorld() && GetWorld()->GetFirstPlayerController())
		? FMath::Abs(GetWorld()->GetFirstPlayerController()->GetInputAxisValue(TEXT("Turn")))
		: 0.f;
}

UCharacterTrajectoryComponent* AAlsasuaCharacter::GetCharacterTrajectory() const
{
	return FindComponentByClass<UCharacterTrajectoryComponent>();
}

// ── ADS ─────────────────────────────────────────────────────────────────────

void AAlsasuaCharacter::ApuntarInicio() { bApuntando = true; }
void AAlsasuaCharacter::ApuntarFin() { bApuntando = false; }

// ── Parkour ─────────────────────────────────────────────────────────────────

void AAlsasuaCharacter::SaltarOTrepar()
{
	if (!IntentarTrepar()) Jump();
}

bool AAlsasuaCharacter::IntentarTrepar()
{
	if (bTrepando) return true;
	const UCapsuleComponent* Cap = GetCapsuleComponent();
	if (!Cap) return false;
	const float h = Cap->GetScaledCapsuleHalfHeight();
	const float r = Cap->GetScaledCapsuleRadius();
	const FVector Fwd = GetActorForwardVector();
	const FVector Loc = GetActorLocation();
	UWorld* W = GetWorld();
	if (!W) return false;

	FCollisionQueryParams Q(SCENE_QUERY_STAT(Trepa), false); Q.AddIgnoredActor(this);

	FHitResult Pared;
	const FVector A0 = Loc + FVector(0, 0, -h * 0.3f);
	if (!W->LineTraceSingleByChannel(Pared, A0, A0 + Fwd * (r + AlcanceTrepa), ECC_Visibility, Q))
		return false;
	if (Pared.ImpactNormal.Z > 0.5f) return false;

	const FVector Cima = Pared.ImpactPoint + Fwd * (r + 20.f) + FVector(0, 0, AlturaTrepaMax + 50.f);
	FHitResult Top;
	if (!W->LineTraceSingleByChannel(Top, Cima, Cima - FVector(0, 0, AlturaTrepaMax + 80.f), ECC_Visibility, Q))
		return false;
	if (Top.ImpactNormal.Z < 0.6f) return false;

	const float Subida = Top.ImpactPoint.Z - (Loc.Z - h);
	if (Subida < 40.f || Subida > AlturaTrepaMax) return false;

	TrepaInicio = Loc;
	TrepaFin = Top.ImpactPoint + Fwd * (r + 10.f) + FVector(0, 0, h + 5.f);
	TrepaT = 0.f;
	TrepaDur = FMath::GetMappedRangeValueClamped(FVector2D(40.f, AlturaTrepaMax), FVector2D(0.3f, 0.55f), Subida);
	bTrepando = true;
	if (UCharacterMovementComponent* Mv = GetCharacterMovement()) Mv->SetMovementMode(MOVE_Flying);
	return true;
}

// ── Enhanced Input ──────────────────────────────────────────────────────────

void AAlsasuaCharacter::AsegurarInputRuntime()
{
	if (bInputRuntimeHecho || ContextoMapeo) return;
	bInputRuntimeHecho = true;

	auto NuevaIA = [this](EInputActionValueType Tipo) -> UInputAction*
	{ UInputAction* A = NewObject<UInputAction>(this); A->ValueType = Tipo; return A; };

	IA_Mover     = NuevaIA(EInputActionValueType::Axis2D);
	IA_Mirar     = NuevaIA(EInputActionValueType::Axis2D);
	IA_Saltar    = NuevaIA(EInputActionValueType::Boolean);
	IA_Correr    = NuevaIA(EInputActionValueType::Boolean);
	IA_Agacharse = NuevaIA(EInputActionValueType::Boolean);

	UInputMappingContext* IMC = NewObject<UInputMappingContext>(this);

	auto MakeSwz = [this]() -> UInputModifierSwizzleAxis*
	{ auto* m = NewObject<UInputModifierSwizzleAxis>(this); m->Order = EInputAxisSwizzle::YXZ; return m; };

	IMC->MapKey(IA_Mover, EKeys::W).Modifiers.Add(MakeSwz());
	{ auto& k = IMC->MapKey(IA_Mover, EKeys::S); k.Modifiers.Add(MakeSwz()); k.Modifiers.Add(NewObject<UInputModifierNegate>(this)); }
	IMC->MapKey(IA_Mover, EKeys::D);
	IMC->MapKey(IA_Mover, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(this));
	IMC->MapKey(IA_Mover, EKeys::Gamepad_Left2D);

	IMC->MapKey(IA_Mirar, EKeys::Mouse2D);
	{ auto& g = IMC->MapKey(IA_Mirar, EKeys::Gamepad_Right2D);
	  auto* sc = NewObject<UInputModifierScalar>(this); sc->Scalar = FVector(2.f, 2.f, 1.f); g.Modifiers.Add(sc); }

	IMC->MapKey(IA_Saltar, EKeys::SpaceBar);
	IMC->MapKey(IA_Saltar, EKeys::Gamepad_FaceButton_Bottom);
	IMC->MapKey(IA_Correr, EKeys::LeftShift);
	IMC->MapKey(IA_Correr, EKeys::Gamepad_LeftShoulder);
	IMC->MapKey(IA_Agacharse, EKeys::C);
	IMC->MapKey(IA_Agacharse, EKeys::Gamepad_FaceButton_Right);

	IA_Apuntar = NuevaIA(EInputActionValueType::Boolean);
	IMC->MapKey(IA_Apuntar, EKeys::RightMouseButton);
	IMC->MapKey(IA_Apuntar, EKeys::Gamepad_LeftTrigger);

	ContextoMapeo = IMC;
}

void AAlsasuaCharacter::EntradaMover(const FInputActionValue& V)
{
	if (!Controller) return;
	const FVector2D E = V.Get<FVector2D>();
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), E.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), E.X);
}

void AAlsasuaCharacter::EntradaMirar(const FInputActionValue& V)
{
	const FVector2D E = V.Get<FVector2D>();
	AddControllerYawInput(E.X);
	AddControllerPitchInput(E.Y);
}

void AAlsasuaCharacter::CorrerInicio()
{
	bCorriendo = true;
	if (UCharacterMovementComponent* Mv = GetCharacterMovement()) Mv->MaxWalkSpeed = VelCorrer;
}

void AAlsasuaCharacter::CorrerFin()
{
	bCorriendo = false;
	if (UCharacterMovementComponent* Mv = GetCharacterMovement()) Mv->MaxWalkSpeed = VelCaminar;
}

void AAlsasuaCharacter::AgacharseToggle()
{
	if (bIsCrouched) UnCrouch(); else Crouch();
}

// ── Fallback clásico ────────────────────────────────────────────────────────

void AAlsasuaCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), Value);
	}
}

void AAlsasuaCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), Value);
	}
}

void AAlsasuaCharacter::TurnAt(float Value)   { AddControllerYawInput(Value * Sensitivity); }
void AAlsasuaCharacter::LookUpAt(float Value) { AddControllerPitchInput(Value * Sensitivity); }
