#include "AI/AlsasuaSquadManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UAlsasuaSquadManager::Tick(float DeltaTime)
{
    // Solo ejecutamos lógica táctica pesada cada medio segundo para ahorrar CPU
    static float Timer = 0;
    Timer += DeltaTime;
    if (Timer < 0.5f) return;
    Timer = 0;

    switch (CurrentTactic)
    {
        case ESquadTactic::Encircle:
            ExecuteEncircleTactic();
            break;
    }
}

void UAlsasuaSquadManager::RegisterUnit(AAlsasuaAIController* Unit)
{
    if (Unit) ActiveUnits.AddUnique(Unit);
}

void UAlsasuaSquadManager::SetGlobalTactic(ESquadTactic NewTactic)
{
    CurrentTactic = NewTactic;
    UE_LOG(LogTemp, Log, TEXT("AI Tactical Shift: %d"), (uint8)NewTactic);
}

void UAlsasuaSquadManager::ExecuteEncircleTactic()
{
    APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!Player || ActiveUnits.Num() == 0) return;

    FVector PlayerLoc = Player->GetActorLocation();
    float Radius = 1500.f; // Radio del cordón policial
    float AngleStep = 360.f / ActiveUnits.Num();

    for (int32 i = 0; i < ActiveUnits.Num(); i++)
    {
        if (ActiveUnits[i].IsValid())
        {
            float Angle = FMath::DegreesToRadians(i * AngleStep);
            FVector TargetPos = PlayerLoc + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0);

            // Enviar orden de movimiento táctico a la unidad
            ActiveUnits[i]->MoveToLocation(TargetPos, 50.f);
        }
    }
}
