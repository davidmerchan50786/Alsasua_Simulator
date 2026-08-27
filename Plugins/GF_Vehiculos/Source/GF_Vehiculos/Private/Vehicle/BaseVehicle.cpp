#include "Vehicle/BaseVehicle.h"
#include "Components/BoxComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/FloatingPawnMovement.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/OverlapResult.h"
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

        // Enable physics and launch vehicle into the air (Carrero Blanco style).
        if (VehicleMesh)
        {
            VehicleMesh->SetSimulatePhysics(true);
            VehicleMesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            // Massive upward + forward impulse — vehicle flies 15-20m high.
            const FVector UpImpulse = FVector(0, 0, 120000) + GetActorForwardVector() * 20000.f;
            VehicleMesh->AddImpulse(UpImpulse, NAME_None, true);
            // Wild spin — random angular impulse.
            VehicleMesh->AddAngularImpulseInDegrees(
                FVector(FMath::RandRange(-150.f, 150.f), FMath::RandRange(-150.f, 150.f), FMath::RandRange(300.f, 600.f)),
                NAME_None, true);
        }

        // Radial blast — launch nearby pawns.
        if (UWorld* W = GetWorld())
        {
            TArray<FOverlapResult> Overlaps;
            FCollisionShape Shape = FCollisionShape::MakeSphere(1500.f);
            if (W->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity, ECC_Pawn, Shape))
            {
                for (const FOverlapResult& Ov : Overlaps)
                {
                    AActor* Target = Ov.GetActor();
                    if (!Target || Target == this) continue;
                    const FVector Dir = (Target->GetActorLocation() - GetActorLocation()).GetSafeNormal();
                    const float Dist = FVector::Dist(GetActorLocation(), Target->GetActorLocation());
                    const float Force = FMath::Max(0.f, 80000.f * (1.f - Dist / 1500.f));
                    if (APawn* P = Cast<APawn>(Target))
                    {
                        if (UCharacterMovementComponent* CM = P->FindComponentByClass<UCharacterMovementComponent>())
                        {
                            CM->SetMovementMode(MOVE_Falling);
                            CM->Velocity = Dir * Force * 0.01f + FVector(0, 0, 800.f);
                        }
                    }
                    if (IDamageable* Dmg = Cast<IDamageable>(Target))
                    {
                        const int32 BlastDmg = FMath::RoundToInt32(200.f * (1.f - Dist / 1500.f));
                        if (BlastDmg > 0) Dmg->RecibirDano(BlastDmg, GetActorLocation(), ETipoDano::Explosion);
                    }
                }
            }
        }

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

        SetLifeSpan(5.0f);
    }
}

void ABaseVehicle::DetonateCarBomb()
{
    RecibirDano(VidaMaxima, GetActorLocation(), ETipoDano::Explosion);
}
