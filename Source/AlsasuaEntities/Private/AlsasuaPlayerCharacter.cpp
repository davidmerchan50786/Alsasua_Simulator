#include "AlsasuaPlayerCharacter.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/AnimInstance.h"
#include "GameFramework/PlayerController.h"
#include "Engine/LocalPlayer.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "EnhancedInputComponent.h"
#include "EnhancedInputSubsystems.h"
#include "InputMappingContext.h"
#include "InputAction.h"
#include "InputModifiers.h"
#include "UObject/ConstructorHelpers.h"

AAlsasuaPlayerCharacter::AAlsasuaPlayerCharacter()
{
	PrimaryActorTick.bCanEverTick = true;   // interpolación de la cámara al apuntar

	GetCapsuleComponent()->InitCapsuleSize(40.f, 90.f);

	// Cuerpo: usa mesh/anim BP del proyecto si existen; si no, deja el fallback del Character.
	if (USkeletalMeshComponent* M = GetMesh())
	{
		M->SetRelativeLocationAndRotation(FVector(0.f, 0.f, -90.f), FRotator(0.f, -90.f, 0.f));

		static ConstructorHelpers::FObjectFinder<USkeletalMesh> SkelMesh(
			TEXT("/Game/Characters/Mannequins/Meshes/SK_Mannequin.SK_Mannequin"));
		if (SkelMesh.Succeeded())
		{
			M->SetSkeletalMesh(SkelMesh.Object);
		}

		static ConstructorHelpers::FClassFinder<UAnimInstance> AnimBP(
			TEXT("/Game/Characters/Mannequins/Anims/Unarmed/ABP_Unarmed"));
		if (AnimBP.Succeeded())
		{
			M->SetAnimInstanceClass(AnimBP.Class);
		}
	}

	// Movimiento AAA: el personaje orienta hacia el avance; cámara lleva el yaw; puede agacharse.
	bUseControllerRotationYaw = false;
	if (UCharacterMovementComponent* Mv = GetCharacterMovement())
	{
		Mv->bOrientRotationToMovement   = true;
		Mv->RotationRate                = FRotator(0.f, 540.f, 0.f);
		Mv->MaxWalkSpeed                = VelCaminar;
		Mv->MaxAcceleration             = 1500.f;
		Mv->BrakingDecelerationWalking  = 2000.f;
		Mv->MinAnalogWalkSpeed          = 20.f;
		Mv->NavAgentProps.bCanCrouch    = true;
		Mv->JumpZVelocity               = 500.f;
		Mv->AirControl                  = 0.35f;
	}

	SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
	SpringArm->SetupAttachment(RootComponent);
	SpringArm->TargetArmLength      = 350.f;          // cm
	SpringArm->SocketOffset         = FVector(0.f, 40.f, 60.f); // sobre el hombro
	SpringArm->bUsePawnControlRotation = true;
	SpringArm->bEnableCameraLag     = true;
	SpringArm->CameraLagSpeed       = 12.f;
	SpringArm->bDoCollisionTest     = true;

	Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
	Camera->SetupAttachment(SpringArm, USpringArmComponent::SocketName);
	Camera->bUsePawnControlRotation = false;
}

void AAlsasuaPlayerCharacter::AsegurarInputRuntime()
{
	if (bInputRuntimeHecho || ContextoMapeo) return;   // ya hay assets de editor o ya construido
	bInputRuntimeHecho = true;

	auto NuevaIA = [&](EInputActionValueType Tipo)
	{ UInputAction* A = NewObject<UInputAction>(this); A->ValueType = Tipo; return A; };

	IA_Mover     = NuevaIA(EInputActionValueType::Axis2D);
	IA_Mirar     = NuevaIA(EInputActionValueType::Axis2D);
	IA_Saltar    = NuevaIA(EInputActionValueType::Boolean);
	IA_Correr    = NuevaIA(EInputActionValueType::Boolean);
	IA_Agacharse = NuevaIA(EInputActionValueType::Boolean);

	UInputMappingContext* IMC = NewObject<UInputMappingContext>(this);

	// MOVER: WASD (1D -> 2D vía swizzle/negate) + stick izquierdo (2D nativo).
	{
		auto MakeSwz = [this]() -> UInputModifierSwizzleAxis* {
			auto* m = NewObject<UInputModifierSwizzleAxis>(this);
			m->Order = EInputAxisSwizzle::YXZ;
			return m;
		};
		IMC->MapKey(IA_Mover, EKeys::W).Modifiers.Add(MakeSwz());                                   // (0,+1)
		{ auto& k = IMC->MapKey(IA_Mover, EKeys::S); k.Modifiers.Add(MakeSwz()); k.Modifiers.Add(NewObject<UInputModifierNegate>(this)); } // (0,-1)
		IMC->MapKey(IA_Mover, EKeys::D);                                                        // (+1,0)
		IMC->MapKey(IA_Mover, EKeys::A).Modifiers.Add(NewObject<UInputModifierNegate>(this));   // (-1,0)
		IMC->MapKey(IA_Mover, EKeys::Gamepad_Left2D);
	}

	// MIRAR: ratón (2D) + stick derecho (2D, con escala de sensibilidad).
	{
		IMC->MapKey(IA_Mirar, EKeys::Mouse2D);
		auto& g = IMC->MapKey(IA_Mirar, EKeys::Gamepad_Right2D);
		auto* sc = NewObject<UInputModifierScalar>(this); sc->Scalar = FVector(2.f, 2.f, 1.f); g.Modifiers.Add(sc);
	}

	// SALTAR / CORRER / AGACHARSE: tecla + botón de mando.
	IMC->MapKey(IA_Saltar,    EKeys::SpaceBar);
	IMC->MapKey(IA_Saltar,    EKeys::Gamepad_FaceButton_Bottom);
	IMC->MapKey(IA_Correr,    EKeys::LeftShift);
	IMC->MapKey(IA_Correr,    EKeys::Gamepad_LeftShoulder);
	IMC->MapKey(IA_Agacharse, EKeys::C);
	IMC->MapKey(IA_Agacharse, EKeys::Gamepad_FaceButton_Right);

	// APUNTAR: ratón derecho + gatillo izquierdo.
	IA_Apuntar = NuevaIA(EInputActionValueType::Boolean);
	IMC->MapKey(IA_Apuntar, EKeys::RightMouseButton);
	IMC->MapKey(IA_Apuntar, EKeys::Gamepad_LeftTrigger);

	ContextoMapeo = IMC;
}

void AAlsasuaPlayerCharacter::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

	// Trepa en curso: interpola al borde superior y luego restaura.
	if (bTrepando)
	{
		TrepaT += DeltaTime;
		const float a = FMath::Clamp(TrepaT / TrepaDur, 0.f, 1.f);
		const float s = FMath::SmoothStep(0.f, 1.f, a);
		SetActorLocation(FMath::Lerp(TrepaInicio, TrepaFin, s));
		if (a >= 1.f)
		{
			bTrepando = false;
			if (UCharacterMovementComponent* Mv = GetCharacterMovement()) Mv->SetMovementMode(MOVE_Walking);
		}
		return;
	}

	if (!SpringArm || !Camera) return;

	// Velocidad real (para que el sprint-FOV solo entre si te mueves de verdad).
	const bool bMov = GetVelocity().SizeSquared2D() > 100.f * 100.f;
	float kBrazo, kFOV;
	if (bApuntando)                 { kBrazo = BrazoApuntar; kFOV = FOVApuntar; }
	else if (bCorriendo && bMov)    { kBrazo = BrazoCorrer;  kFOV = FOVCorrer; }
	else                            { kBrazo = BrazoCadera;  kFOV = FOVCadera; }

	SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, kBrazo, DeltaTime, 8.f);
	Camera->FieldOfView        = FMath::FInterpTo(Camera->FieldOfView, kFOV, DeltaTime, 8.f);
}

void AAlsasuaPlayerCharacter::ApuntarInicio() { bApuntando = true; }
void AAlsasuaPlayerCharacter::ApuntarFin()    { bApuntando = false; }

void AAlsasuaPlayerCharacter::RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo)
{
	Vida = FMath::Max(0, Vida - Cantidad);

	// Flinch: sacudida de cámara al encajar el golpe (más fuerte cuanto más daño).
	if (APlayerController* PC = Cast<APlayerController>(GetController()))
	{
		const float k = FMath::Clamp(Cantidad / 15.f, 0.3f, 2.f);
		PC->AddPitchInput(FMath::FRandRange(-0.9f, -0.2f) * k);   // cabezada hacia arriba
		PC->AddYawInput(FMath::FRandRange(-0.7f, 0.7f) * k);
	}
}

void AAlsasuaPlayerCharacter::BeginPlay()
{
	Super::BeginPlay();
	AsegurarInputRuntime();
	if (const APlayerController* PC = Cast<APlayerController>(GetController()))
		if (UEnhancedInputLocalPlayerSubsystem* Sub =
				ULocalPlayer::GetSubsystem<UEnhancedInputLocalPlayerSubsystem>(PC->GetLocalPlayer()))
			if (ContextoMapeo) Sub->AddMappingContext(ContextoMapeo, 0);
}

void AAlsasuaPlayerCharacter::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
	Super::SetupPlayerInputComponent(PlayerInputComponent);

	AsegurarInputRuntime();   // crea las acciones si no hay assets de editor

	// Preferente: Enhanced Input (si hay assets asignados en el Blueprint).
	if (UEnhancedInputComponent* EIC = Cast<UEnhancedInputComponent>(PlayerInputComponent))
	{
		if (IA_Mover)  EIC->BindAction(IA_Mover, ETriggerEvent::Triggered, this, &AAlsasuaPlayerCharacter::EntradaMover);
		if (IA_Mirar)  EIC->BindAction(IA_Mirar, ETriggerEvent::Triggered, this, &AAlsasuaPlayerCharacter::EntradaMirar);
		if (IA_Saltar)
		{
			EIC->BindAction(IA_Saltar, ETriggerEvent::Started,   this, &AAlsasuaPlayerCharacter::SaltarOTrepar);
			EIC->BindAction(IA_Saltar, ETriggerEvent::Completed, this, &ACharacter::StopJumping);
		}
		if (IA_Correr)
		{
			EIC->BindAction(IA_Correr, ETriggerEvent::Started,   this, &AAlsasuaPlayerCharacter::CorrerInicio);
			EIC->BindAction(IA_Correr, ETriggerEvent::Completed, this, &AAlsasuaPlayerCharacter::CorrerFin);
		}
		if (IA_Agacharse) EIC->BindAction(IA_Agacharse, ETriggerEvent::Started, this, &AAlsasuaPlayerCharacter::AgacharseToggle);
		if (IA_Apuntar)
		{
			EIC->BindAction(IA_Apuntar, ETriggerEvent::Started,   this, &AAlsasuaPlayerCharacter::ApuntarInicio);
			EIC->BindAction(IA_Apuntar, ETriggerEvent::Completed, this, &AAlsasuaPlayerCharacter::ApuntarFin);
		}
	}

	// Fallback clásico (siempre activo; AxisMappings de DefaultInput.ini).
	PlayerInputComponent->BindAxis(TEXT("MoveForward"), this, &AAlsasuaPlayerCharacter::MoveForward);
	PlayerInputComponent->BindAxis(TEXT("MoveRight"),   this, &AAlsasuaPlayerCharacter::MoveRight);
	PlayerInputComponent->BindAxis(TEXT("Turn"),        this, &AAlsasuaPlayerCharacter::TurnAt);
	PlayerInputComponent->BindAxis(TEXT("LookUp"),      this, &AAlsasuaPlayerCharacter::LookUpAt);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Pressed,  this, &ACharacter::Jump);
	PlayerInputComponent->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
}

// ── Enhanced Input ──────────────────────────────────────────────────────────
void AAlsasuaPlayerCharacter::EntradaMover(const FInputActionValue& V)
{
	if (!Controller) return;
	const FVector2D E = V.Get<FVector2D>();
	const FRotator Yaw(0.f, Controller->GetControlRotation().Yaw, 0.f);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::X), E.Y);
	AddMovementInput(FRotationMatrix(Yaw).GetUnitAxis(EAxis::Y), E.X);
}

void AAlsasuaPlayerCharacter::EntradaMirar(const FInputActionValue& V)
{
	const FVector2D E = V.Get<FVector2D>();
	AddControllerYawInput(E.X);
	AddControllerPitchInput(E.Y);
}

void AAlsasuaPlayerCharacter::SaltarOTrepar() { if (!IntentarTrepar()) Jump(); }

bool AAlsasuaPlayerCharacter::IntentarTrepar()
{
	if (bTrepando) return true;
	const UCapsuleComponent* Cap = GetCapsuleComponent();
	if (!Cap) return false;
	const float h = Cap->GetScaledCapsuleHalfHeight();
	const float r = Cap->GetScaledCapsuleRadius();
	const FVector Fwd = GetActorForwardVector();
	const FVector Loc = GetActorLocation();

	FCollisionQueryParams Q(SCENE_QUERY_STAT(Trepa), false); Q.AddIgnoredActor(this);

	// 1) ¿hay un muro/repecho delante a la altura del pecho?
	FHitResult Pared;
	const FVector A0 = Loc + FVector(0, 0, -h * 0.3f);
	if (!GetWorld()->LineTraceSingleByChannel(Pared, A0, A0 + Fwd * (r + AlcanceTrepa), ECC_Visibility, Q))
		return false;
	if (Pared.ImpactNormal.Z > 0.5f) return false;   // es suelo, no pared

	// 2) buscar el borde superior: trazar hacia abajo desde encima del muro.
	const FVector Cima = Pared.ImpactPoint + Fwd * (r + 20.f) + FVector(0, 0, AlturaTrepaMax + 50.f);
	FHitResult Top;
	if (!GetWorld()->LineTraceSingleByChannel(Top, Cima, Cima - FVector(0, 0, AlturaTrepaMax + 80.f), ECC_Visibility, Q))
		return false;
	if (Top.ImpactNormal.Z < 0.6f) return false;   // la cima debe ser pisable

	const float Subida = Top.ImpactPoint.Z - (Loc.Z - h);   // altura del repecho
	if (Subida < 40.f || Subida > AlturaTrepaMax) return false;

	// 3) trepar: destino encima del borde.
	TrepaInicio = Loc;
	TrepaFin    = Top.ImpactPoint + Fwd * (r + 10.f) + FVector(0, 0, h + 5.f);
	TrepaT = 0.f;
	TrepaDur = FMath::GetMappedRangeValueClamped(FVector2D(40.f, AlturaTrepaMax), FVector2D(0.3f, 0.55f), Subida);
	bTrepando = true;
	if (UCharacterMovementComponent* Mv = GetCharacterMovement()) Mv->SetMovementMode(MOVE_Flying);   // sin gravedad durante la trepa
	return true;
}

void AAlsasuaPlayerCharacter::CorrerInicio() { bCorriendo = true;  if (UCharacterMovementComponent* Mv = GetCharacterMovement()) Mv->MaxWalkSpeed = VelCorrer; }
void AAlsasuaPlayerCharacter::CorrerFin()    { bCorriendo = false; if (UCharacterMovementComponent* Mv = GetCharacterMovement()) Mv->MaxWalkSpeed = VelCaminar; }
void AAlsasuaPlayerCharacter::AgacharseToggle() { if (bIsCrouched) UnCrouch(); else Crouch(); }

// ── Fallback clásico ────────────────────────────────────────────────────────
void AAlsasuaPlayerCharacter::MoveForward(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator Y(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Y).GetUnitAxis(EAxis::X), Value);
	}
}

void AAlsasuaPlayerCharacter::MoveRight(float Value)
{
	if (Controller && Value != 0.f)
	{
		const FRotator Y(0.f, Controller->GetControlRotation().Yaw, 0.f);
		AddMovementInput(FRotationMatrix(Y).GetUnitAxis(EAxis::Y), Value);
	}
}

void AAlsasuaPlayerCharacter::TurnAt(float Value)   { AddControllerYawInput(Value); }
void AAlsasuaPlayerCharacter::LookUpAt(float Value) { AddControllerPitchInput(Value); }
