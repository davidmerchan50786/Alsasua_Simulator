#include "Vehicle/BaseVehicle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "NiagaraSystem.h"
#include "NiagaraFunctionLibrary.h"

ABaseVehicle::ABaseVehicle()
{
    PrimaryActorTick.bCanEverTick = true;

    CollisionBox = CreateDefaultSubobject<UBoxComponent>(TEXT("CollisionBox"));
    RootComponent = CollisionBox;

    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    VehicleMesh->SetupAttachment(RootComponent);

    MovementComponent = CreateDefaultSubobject<UFloatingPawnMovement>(TEXT("Movement"));
    MovementComponent->MaxSpeed = MaxSpeed;
}

void ABaseVehicle::BeginPlay()
{
    Super::BeginPlay();
    // Niagara y NS_ExplosionCoche, no Cascade y P_Explosion. P_Explosion no lo
    // crea nadie —Tools/create_niagara_vfx.py fabrica NS_*, que es lo que usan
    // VehiculoJugable, VehiculoAmbiente y ArmasComponent—, así que este
    // LoadObject devolvía null y los vehículos de esta clase explotaban sin
    // efecto. Era el único sitio del proyecto que seguía en Cascade.
    CachedExplosionVFX = LoadObject<UNiagaraSystem>(
        nullptr, TEXT("/Game/VFX/NS_ExplosionCoche.NS_ExplosionCoche"));
    CachedExplosionSFX = LoadObject<USoundBase>(
        nullptr, TEXT("/Game/Audio/SC_Explosion.SC_Explosion"));
}

void ABaseVehicle::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    CurrentSpeedKmh = GetVelocity().Size() * 0.036f;
}

void ABaseVehicle::Drive(float ForwardValue, float RightValue)
{
    if (!bEngineActive || bDestruido) return;

    AddMovementInput(GetActorForwardVector(), ForwardValue);

    if (GetVelocity().Size() > 100.f)
    {
        float TurnDir = ForwardValue >= 0 ? 1.f : -1.f;
        AddActorLocalRotation(FRotator(0.f, RightValue * TurnSpeed * GetWorld()->GetDeltaSeconds() * TurnDir, 0.f));
    }
}

void ABaseVehicle::ToggleEngine(bool bOn)
{
    bEngineActive = bOn;
}

void ABaseVehicle::RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo)
{
    if (bDestruido) return;

    Vida = FMath::Max(0, Vida - Cantidad);

    if (Vida <= 0)
    {
        bDestruido = true;
        bEngineActive = false;

        if (MovementComponent) MovementComponent->StopMovementImmediately();

        // Detonate with explosion effect.
        if (CachedExplosionVFX)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(GetWorld(),
                CachedExplosionVFX,
                GetActorLocation(), GetActorRotation());
        }
        UGameplayStatics::PlaySoundAtLocation(GetWorld(),
            CachedExplosionSFX,
            GetActorLocation());

        SetLifeSpan(3.0f);
    }
}

void ABaseVehicle::DetonateCarBomb()
{
    RecibirDano(VidaMaxima, GetActorLocation(), ETipoDano::Explosion);
}
