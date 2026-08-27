#include "World/Barricade/AlsasuaBarricadeActor.h"
#include "Components/StaticMeshComponent.h"
#include "Particles/ParticleSystemComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AAlsasuaBarricadeActor::AAlsasuaBarricadeActor()
{
    PrimaryActorTick.bCanEverTick = true;

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Mesh"));
    RootComponent = MeshComp;

    static ConstructorHelpers::FObjectFinder<UStaticMesh> ContenedorMesh(
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (ContenedorMesh.Succeeded())
        MeshComp->SetStaticMesh(ContenedorMesh.Object);

    MeshComp->SetCollisionProfileName(TEXT("BlockAll"));
    MeshComp->SetCastShadow(true);

    CurrentHealth = MaxHealth;
}

void AAlsasuaBarricadeActor::BeginPlay()
{
    Super::BeginPlay();

    switch (Tipo)
    {
    case EBarricadeType::Contenedor:
        SetActorScale3D(FVector(2.0f, 4.0f, 1.5f));
        break;
    case EBarricadeType::Coche:
        SetActorScale3D(FVector(4.0f, 2.0f, 1.3f));
        break;
    case EBarricadeType::Neumaticos:
        SetActorScale3D(FVector(1.5f, 1.5f, 1.8f));
        break;
    case EBarricadeType::Escombros:
        SetActorScale3D(FVector(3.0f, 3.0f, 1.0f));
        break;
    case EBarricadeType::BarbacoaGrill:
        SetActorScale3D(FVector(1.0f, 1.0f, 1.2f));
        PrenderFuego();
        break;
    }
}

void AAlsasuaBarricadeActor::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsBurning)
    {
        BurnTimer += DeltaTime;
        if (BurnTimer >= BurnDuration)
        {
            DestruirBarricada();
            return;
        }

        // Flicker intensity
        if (FireVFX)
        {
            const float Intensity = 1.0f - (BurnTimer / BurnDuration);
            FireVFX->SetFloatParameter(TEXT("Intensity"), Intensity);
        }
    }
}

void AAlsasuaBarricadeActor::RecibirDano(float Cantidad)
{
    CurrentHealth -= Cantidad;
    if (CurrentHealth <= 0.f)
    {
        DestruirBarricada();
    }
}

void AAlsasuaBarricadeActor::PrenderFuego()
{
    if (bIsBurning) return;
    bIsBurning = true;
    BurnTimer = 0.f;

    UWorld* W = GetWorld();
    if (!W) return;

    // Try to spawn fire VFX
    UNiagaraSystem* FireNS = LoadObject<UNiagaraSystem>(nullptr,
        TEXT("/Game/VFX/NS_Fuego.NS_Fuego"));
    if (FireNS)
    {
        FireVFX = UNiagaraFunctionLibrary::SpawnSystemAttached(
            FireNS, RootComponent, NAME_None, FVector(0, 0, 50.f),
            FRotator::ZeroRotator, EAttachLocation::KeepRelativeOffset, true);
    }

    // Light damage to nearby NPCs
    TArray<AActor*> NearbyActors;
    UGameplayStatics::GetAllActorsOfClass(W, ACharacter::StaticClass(), NearbyActors);
    for (AActor* A : NearbyActors)
    {
        if (FVector::Dist(GetActorLocation(), A->GetActorLocation()) < 300.f)
        {
            // Flee effect — apply damage to make them run
            UGameplayStatics::ApplyDamage(A, 1.f, nullptr, this, nullptr);
        }
    }
}

void AAlsasuaBarricadeActor::DestruirBarricada()
{
    if (FireVFX)
    {
        FireVFX->DeactivateImmediate();
        FireVFX->DestroyComponent();
    }

    // Spawn debris particles on destruction
    UWorld* W = GetWorld();
    if (W)
    {
        UNiagaraSystem* DebrisNS = LoadObject<UNiagaraSystem>(nullptr,
            TEXT("/Game/VFX/NS_Debritos.NS_Debritos"));
        if (DebrisNS)
        {
            UNiagaraFunctionLibrary::SpawnSystemAtLocation(
                W, DebrisNS, GetActorLocation(), FRotator::ZeroRotator,
                FVector(1.5f));
        }
    }

    Destroy();
}
