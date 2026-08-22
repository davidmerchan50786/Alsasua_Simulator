#include "Vehicles/VehicleTarget.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

AVehicleTarget::AVehicleTarget()
{
    PrimaryActorTick.bCanEverTick = false;

    VehicleMesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    SetRootComponent(VehicleMesh);
}

void AVehicleTarget::BeginPlay()
{
    Super::BeginPlay();
}

bool AVehicleTarget::ReceiveRadialImpulse(const FVector& Origin, float Force, float Damage)
{
    CurrentHealth = FMath::Max(0.f, CurrentHealth - Damage);

    if (VehicleMesh)
    {
        FVector Dir = (GetActorLocation() - Origin).GetSafeNormal();
        VehicleMesh->AddImpulse(Dir * Force, NAME_None, true);
    }

    if (CurrentHealth <= 0.f)
    {
        OnDestroyedByExplosion();
        return true;
    }

    return false;
}

void AVehicleTarget::IgniteAtLocation(const FVector& Loc, UParticleSystem* FireFX, float Duration)
{
    if (FireFX)
    {
        UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), FireFX, Loc);
    }

    if (VehicleMesh)
    {
        VehicleMesh->SetSimulatePhysics(true);
    }
}

void AVehicleTarget::OnDestroyedByExplosion()
{
    if (VehicleMesh)
    {
        UMaterialInstanceDynamic* DynMat = VehicleMesh->CreateAndSetMaterialInstanceDynamic(0);
        if (DynMat)
        {
            DynMat->SetScalarParameterValue("BurnAmount", 1.0f);
            DynMat->SetVectorParameterValue("CharColor", FLinearColor::Black);
        }

        VehicleMesh->AddImpulse(FVector(0, 0, 150000), NAME_None, true);
        VehicleMesh->AddAngularImpulseInDegrees(FVector(FMath::RandRange(-100, 100), FMath::RandRange(-100, 100), 500), NAME_None, true);
    }
}
