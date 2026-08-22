#include "AI/Vehicle/PursuitSubsystem.h"
#include "Engine/World.h"

void UPursuitSubsystem::RegisterPatrolAction(bool bJoined)
{
    CurrentPatrolsInChase = FMath::Clamp(CurrentPatrolsInChase + (bJoined ? 1 : -1), 0, MaxPatrols);
}

void UPursuitSubsystem::RequestBackup(FVector Location, AActor* Target)
{
    if (CurrentPatrolsInChase >= MaxPatrols) return;

    if (PatrolVehicleClass)
    {
        UWorld* W = GetWorld();
        if (!W) return;

        FActorSpawnParameters Params;
        // Spawnear a una distancia prudente detrás o en calles laterales
        FVector SpawnPos = Location + FVector(FMath::RandRange(-2000.f, 2000.f), FMath::RandRange(-2000.f, 2000.f), 100.f);

        AActor* NewPatrol = W->SpawnActor<AActor>(PatrolVehicleClass, SpawnPos, FRotator::ZeroRotator, Params);
        if (NewPatrol)
        {
            RegisterPatrolAction(true);
            UE_LOG(LogTemp, Warning, TEXT("¡Refuerzos en camino! Patrullas activas: %d"), CurrentPatrolsInChase);
        }
    }
}
