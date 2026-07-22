#include "AI/Vehicle/VehicleAIController.h"
#include "Vehicle/BaseVehicle.h"
#include "Kismet/GameplayStatics.h"

AVehicleAIController::AVehicleAIController() { PrimaryActorTick.bCanEverTick = true; }

void AVehicleAIController::OnPossess(APawn* InPawn)
{
    Super::OnPossess(InPawn);
}

void AVehicleAIController::StartPursuit(APawn* TargetPawn)
{
    PursuitTarget = TargetPawn;
}

void AVehicleAIController::StopPursuit()
{
    PursuitTarget = nullptr;
    StopMovement();
}

void AVehicleAIController::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);
    if (!PursuitTarget) return;

    APawn* MyPawn = GetPawn();
    if (!MyPawn) return;

    FVector ToTarget = PursuitTarget->GetActorLocation() - MyPawn->GetActorLocation();
    float Dist = ToTarget.Size();

    FVector Dir = ToTarget.GetSafeNormal();
    float Forward = FVector::DotProduct(MyPawn->GetActorForwardVector(), Dir);
    float Right = FVector::CrossProduct(MyPawn->GetActorForwardVector(), Dir).Z > 0 ? 1.f : -1.f;

    // Ajustar valores de conducción en función de la distancia
    float Throttle = FMath::Clamp((1000.f - Dist) / 1000.f * PursuitAggression, 0.3f, 1.f);

    if (ABaseVehicle* V = Cast<ABaseVehicle>(MyPawn))
    {
        V->Drive(Throttle, Right * 0.5f);
    }
}

