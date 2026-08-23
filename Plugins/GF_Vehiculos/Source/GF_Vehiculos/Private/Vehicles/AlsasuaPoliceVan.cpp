#include "Vehicles/AlsasuaPoliceVan.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/KismetMathLibrary.h"

AAlsasuaPoliceVan::AAlsasuaPoliceVan()
{
    PrimaryActorTick.bCanEverTick = true;
    Mesh = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("VehicleMesh"));
    RootComponent = Mesh;

    // Interceptor real si Content/AssetsImportados/PoliceCarHelicopter existe
    // (no se versiona — es "puesta a punto tras clonar", como las ortofotos).
    // Fallback al SUV del VehicleVarietyPack, que sí se copia con el proyecto
    // en la mayoría de los clones, antes de caer a la forma básica del motor.
    UStaticMesh* Malla = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/AssetsImportados/PoliceCarHelicopter/Models/Interceptor.Interceptor"));
    if (!Malla)
        Malla = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Game/VehicleVarietyPack/Meshes/SM_SUV.SM_SUV"));
    if (!Malla)
        Malla = LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (Malla)
        Mesh->SetStaticMesh(Malla);
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
