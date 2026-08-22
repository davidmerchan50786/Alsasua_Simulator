#include "Vehicles/AlsasuaPoliceVan.h"
#include "Components/SkeletalMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

AAlsasuaPoliceVan::AAlsasuaPoliceVan()
{
    PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("VehicleMesh"));
    RootComponent = Mesh;
}

void AAlsasuaPoliceVan::MoveToLocationTactic(FVector TargetLocation)
{
    GoalLocation = TargetLocation;
    bIsMoving = true;
    bSirensActive = true;
    UE_LOG(LogTemp, Warning, TEXT("VAN: Entrada táctica iniciada hacia la Plaza."));
}

void AAlsasuaPoliceVan::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bIsMoving)
    {
        FVector CurrentLoc = GetActorLocation();
        FVector Direction = (GoalLocation - CurrentLoc).GetSafeNormal();
        float Distance = FVector::Dist(CurrentLoc, GoalLocation);

        if (Distance > 100.f)
        {
            SetActorLocation(CurrentLoc + (Direction * TargetSpeed * DeltaTime), true);

            // Orientación suave hacia el objetivo
            FRotator TargetRot = UKismetMathLibrary::FindLookAtRotation(CurrentLoc, GoalLocation);
            SetActorRotation(FMath::RInterpTo(GetActorRotation(), TargetRot, DeltaTime, 2.0f));
        }
        else
        {
            bIsMoving = false;
            bSirensActive = false;
            UE_LOG(LogTemp, Log, TEXT("VAN: Posición de bloqueo alcanzada."));
        }
    }
}
