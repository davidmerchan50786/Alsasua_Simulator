#include "Vehicle/PlayerVehicle.h"
#include "Vehicle/VehicleDamageComponent.h"
#include "GameFramework/SpringArmComponent.h"
#include "Camera/CameraComponent.h"
#include "Components/AudioComponent.h"
#include "Components/InputComponent.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/PawnMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

APlayerVehicle::APlayerVehicle()
{
    SpringArm = CreateDefaultSubobject<USpringArmComponent>(TEXT("SpringArm"));
    SpringArm->SetupAttachment(RootComponent);
    SpringArm->TargetArmLength = 800.f;
    SpringArm->bUsePawnControlRotation = false;
    SpringArm->bInheritPitch = false;
    SpringArm->bInheritYaw = true;
    SpringArm->bInheritRoll = false;
    SpringArm->bDoCollisionTest = true;
    SpringArm->SocketOffset = FVector(0, 0, 200.f);

    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("Camera"));
    Camera->SetupAttachment(SpringArm);

    DamageComponent = CreateDefaultSubobject<UVehicleDamageComponent>(TEXT("DamageComponent"));

    EngineAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("EngineAudio"));
    EngineAudio->SetupAttachment(RootComponent);
    EngineAudio->bAutoActivate = false;

    HornAudio = CreateDefaultSubobject<UAudioComponent>(TEXT("HornAudio"));
    HornAudio->SetupAttachment(RootComponent);
    HornAudio->bAutoActivate = false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input Setup
// ─────────────────────────────────────────────────────────────────────────────
void APlayerVehicle::SetupPlayerInputComponent(UInputComponent* PlayerInputComponent)
{
    Super::SetupPlayerInputComponent(PlayerInputComponent);

    PlayerInputComponent->BindAxis("MoveForward", this, &APlayerVehicle::OnGas);
    PlayerInputComponent->BindAxis("MoveRight", this, &APlayerVehicle::OnSteer);

    // Bind brake to a separate axis if available, otherwise use negative forward.
    PlayerInputComponent->BindAction("Handbrake", IE_Pressed, this, &APlayerVehicle::OnHandbrakePressed);
    PlayerInputComponent->BindAction("Handbrake", IE_Released, this, &APlayerVehicle::OnHandbrakeReleased);

    PlayerInputComponent->BindAction("Horn", IE_Pressed, this, &APlayerVehicle::OnHornPressed);
    PlayerInputComponent->BindAction("Horn", IE_Released, this, &APlayerVehicle::OnHornReleased);
    PlayerInputComponent->BindAction("ToggleEngine", IE_Pressed, this, &APlayerVehicle::OnToggleEngine);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Input Handlers
// ─────────────────────────────────────────────────────────────────────────────
void APlayerVehicle::OnGas(float Value)
{
    if (Value > 0.f)
    {
        GasInput = Value;
        BrakeInput = 0.f;
    }
    else if (Value < 0.f)
    {
        GasInput = 0.f;
        BrakeInput = FMath::Abs(Value);
    }
    else
    {
        GasInput = 0.f;
        BrakeInput = 0.f;
    }
}

void APlayerVehicle::OnSteer(float Value)
{
    SteerInput = Value;
}

void APlayerVehicle::OnHandbrakePressed()
{
    bHandbrakeActive = true;
}

void APlayerVehicle::OnHandbrakeReleased()
{
    bHandbrakeActive = false;
}

void APlayerVehicle::OnHornPressed()
{
    if (HornSound)
    {
        HornAudio->SetSound(HornSound);
        HornAudio->Play();
    }
}

void APlayerVehicle::OnHornReleased()
{
    if (HornAudio && HornAudio->IsPlaying())
    {
        HornAudio->Stop();
    }
}

void APlayerVehicle::OnToggleEngine()
{
    ToggleEngine(!bEngineActive);
    bEngineRunning = bEngineActive;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Enter / Exit
// ─────────────────────────────────────────────────────────────────────────────
void APlayerVehicle::EnterVehicle(APawn* Driver)
{
    if (Driver == nullptr || DriverPawn != nullptr)
    {
        return;
    }

    DriverPawn = Driver;

    // Possesar el vehículo con el controller del driver.
    AController* DriverController = Driver->GetController();
    if (DriverController)
    {
        DriverController->UnPossess();
        DriverController->Possess(this);
    }

    // Ocultar el pawn del driver.
    Driver->SetActorHiddenInGame(true);
    Driver->SetActorEnableCollision(false);
    Driver->SetActorTickEnabled(false);

    // Activar cámara y audio.
    if (Camera) Camera->SetActive(true);
    if (EngineAudio)
    {
        EngineAudio->SetAutoActivate(true);
        EngineAudio->FadeIn(0.5f);
    }

    // Input.
    APlayerController* PC = Cast<APlayerController>(DriverController);
    if (PC)
    {
        PC->SetViewTargetWithBlend(this, 0.3f);
    }

    OnVehicleEntered.Broadcast(this);
}

void APlayerVehicle::ExitVehicle()
{
    if (DriverPawn == nullptr)
    {
        return;
    }

    APawn* Driver = DriverPawn;
    DriverPawn = nullptr;

    // Spawnear el driver detrás del vehículo.
    FVector ExitLocation = GetActorLocation() - GetActorForwardVector() * 200.f;
    FHitResult Hit;
    if (UWorld* W = GetWorld())
    {
        if (W->LineTraceSingleByChannel(Hit, ExitLocation + FVector(0, 0, 200), ExitLocation - FVector(0, 0, 200), ECC_Visibility))
        {
            ExitLocation = Hit.ImpactPoint;
        }
    }
    Driver->SetActorLocation(ExitLocation);
    Driver->SetActorRotation(GetActorRotation());
    Driver->SetActorHiddenInGame(false);
    Driver->SetActorEnableCollision(true);
    Driver->SetActorTickEnabled(true);

    // Devolver control al driver.
    AController* DriverController = GetController();
    if (DriverController)
    {
        DriverController->UnPossess();
        DriverController->Possess(Driver);
        APlayerController* PC = Cast<APlayerController>(DriverController);
        if (PC)
        {
            PC->SetViewTargetWithBlend(Driver, 0.3f);
        }
    }

    // Detener audio.
    if (EngineAudio) EngineAudio->FadeOut(0.5f, 0.f);

    // Freno de mano automático.
    ToggleEngine(false);
    bEngineRunning = false;

    OnVehicleExited.Broadcast(this);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Tick
// ─────────────────────────────────────────────────────────────────────────────
void APlayerVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!IsOccupied())
    {
        return;
    }

    UpdateDrivingPhysics(DeltaTime);
    UpdateCamera(DeltaTime);
    UpdateEngineAudio(DeltaTime);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Driving Physics
// ─────────────────────────────────────────────────────────────────────────────
void APlayerVehicle::UpdateDrivingPhysics(float DeltaTime)
{
    if (!bEngineActive)
    {
        // Freno de mano automático cuando el motor está apagado.
        if (MovementComponent)
        {
            FVector Velocity = GetVelocity();
            float Speed = Velocity.Size();
            if (Speed > 10.f)
            {
                FVector Decel = -Velocity.GetSafeNormal() * HandbrakeForce * DeltaTime;
                Velocity += Decel;
                if (Velocity.SizeSquared() < 100.f) Velocity = FVector::ZeroVector;
                MovementComponent->Velocity = Velocity;
            }
        }
        return;
    }

    if (!MovementComponent)
    {
        return;
    }

    // ── Gas ────────────────────────────────────────────────────────────────
    if (GasInput > 0.f)
    {
        AddMovementInput(GetActorForwardVector(), GasInput);
    }

    // ── Freno ──────────────────────────────────────────────────────────────
    if (BrakeInput > 0.f)
    {
        FVector Velocity = GetVelocity();
        float Speed = Velocity.Size();

        if (Speed > 50.f)
        {
            FVector Decel = -Velocity.GetSafeNormal() * BrakeForce * BrakeInput * DeltaTime;
            Velocity += Decel;

            if (Velocity.SizeSquared() < 2500.f)
            {
                Velocity = FVector::ZeroVector;
            }
        }
        else
        {
            Velocity = FVector::ZeroVector;
        }

        MovementComponent->Velocity = Velocity;
    }

    // ── Dirección ──────────────────────────────────────────────────────────
    const float Speed = GetVelocity().Size();
    const float SpeedFactor = FMath::Clamp(Speed / MaxSpeed, 0.f, 1.f);

    // A mayor velocidad, menor capacidad de giro (realismo).
    const float EffectiveSteer = SteerInput * TurnSpeed * (1.f - SpeedFactor * 0.6f);

    // Suavizado del steering (inercia de dirección).
    const float SteerLerpSpeed = bHandbrakeActive ? 8.f : 4.f;
    CurrentSteerAngle = FMath::FInterpTo(CurrentSteerAngle, EffectiveSteer, DeltaTime, SteerLerpSpeed);

    if (FMath::Abs(CurrentSteerAngle) > 0.1f && Speed > 50.f)
    {
        float TurnDirection = GasInput >= 0.f ? 1.f : -1.f;
        AddActorLocalRotation(FRotator(0.f, CurrentSteerAngle * DeltaTime * TurnDirection, 0.f));
    }

    // ── Freno de mano (derrape) ────────────────────────────────────────────
    if (bHandbrakeActive && Speed > 200.f)
    {
        ApplyDriftPhysics(DeltaTime);
    }
    else
    {
        DriftAngularVelocity = 0.f;
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Drift Physics
// ─────────────────────────────────────────────────────────────────────────────
void APlayerVehicle::ApplyDriftPhysics(float DeltaTime)
{
    FVector Velocity = GetVelocity();
    float Speed = Velocity.Size();

    if (Speed < 100.f) return;

    // Reducir la velocidad lateral (simula pérdida de adherencia).
    FVector Forward = GetActorForwardVector();
    FVector Right = GetActorRightVector();

    float ForwardSpeed = FVector::DotProduct(Velocity, Forward);
    float RightSpeed = FVector::DotProduct(Velocity, Right);

    // Aplicar fricción reducida al componente lateral.
    float LateralFriction = FMath::Lerp(0.01f, 1.f, DriftFriction);
    RightSpeed *= FMath::FInterpTo(1.f, LateralFriction, DeltaTime, 3.f);

    // Reconstruir velocidad.
    FVector NewVelocity = Forward * ForwardSpeed + Right * RightSpeed;
    NewVelocity.Z = Velocity.Z;

    MovementComponent->Velocity = NewVelocity;

    // Ángulo de derrape visual (rotación extra basada en input lateral).
    DriftAngularVelocity = SteerInput * 120.f * (Speed / MaxSpeed);
    AddActorLocalRotation(FRotator(0.f, DriftAngularVelocity * DeltaTime, 0.f));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Camera
// ─────────────────────────────────────────────────────────────────────────────
void APlayerVehicle::UpdateCamera(float DeltaTime)
{
    if (!SpringArm || !Camera) return;

    // La cámara sigue la dirección del vehículo con suavizado.
    const float Speed = GetVelocity().Size();

    // A mayor velocidad, la cámara se aleja ligeramente.
    const float TargetArmLength = FMath::Lerp(600.f, 1000.f, Speed / MaxSpeed);
    SpringArm->TargetArmLength = FMath::FInterpTo(SpringArm->TargetArmLength, TargetArmLength, DeltaTime, 2.f);

    // Offset lateral al girar.
    const float CameraLateralOffset = SteerInput * 100.f * (Speed / MaxSpeed);
    SpringArm->SocketOffset.Y = FMath::FInterpTo(SpringArm->SocketOffset.Y, CameraLateralOffset, DeltaTime, 3.f);
}

// ─────────────────────────────────────────────────────────────────────────────
//  Engine Audio
// ─────────────────────────────────────────────────────────────────────────────
void APlayerVehicle::UpdateEngineAudio(float DeltaTime)
{
    if (!EngineAudio || !bEngineRunning) return;

    float SpeedKmh = CurrentSpeedKmh;
    float PitchTarget;

    if (SpeedKmh < EngineHighRPMThreshold)
    {
        // Rango normal: pitch de 1.0 a 1.8.
        PitchTarget = FMath::Lerp(0.8f, 1.8f, SpeedKmh / EngineHighRPMThreshold);
    }
    else
    {
        // Alto RPM: pitch de 1.8 a MaxPitch.
        float HighRPMFactor = FMath::Clamp((SpeedKmh - EngineHighRPMThreshold) / (MaxSpeed * 0.036f - EngineHighRPMThreshold), 0.f, 1.f);
        PitchTarget = FMath::Lerp(1.8f, EngineMaxPitch, HighRPMFactor);
    }

    // Acelerar más rápido que desacelerar (realismo de motor).
    float InterpSpeed = (PitchTarget > EngineAudio->PitchMultiplier) ? 4.f : 2.f;
    EngineAudio->SetPitchMultiplier(FMath::FInterpTo(EngineAudio->PitchMultiplier, PitchTarget, DeltaTime, InterpSpeed));
}
