#include "AI/AlsasuaSquadManager.h"
#include "AI/AlsasuaAIController.h"
#include "Core/AlsasuaProfiling.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/Character.h"

void UAlsasuaSquadManager::Tick(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_AlsasuaAI_SquadTick);
    TacticTimer += DeltaTime;
    if (TacticTimer < 0.5f) return;
    TacticTimer = 0;

    switch (CurrentTactic)
    {
        case ESquadTactic::Encircle:
            ExecuteEncircleTactic();
            break;
        case ESquadTactic::Contain:
            ExecuteContainTactic();
            break;
        case ESquadTactic::Patrol:
            ExecutePatrolTactic();
            break;
        case ESquadTactic::Support:
            ExecuteSupportTactic();
            break;
    }
}

void UAlsasuaSquadManager::RegisterUnit(AAlsasuaAIController* Unit)
{
    if (Unit) ActiveUnits.AddUnique(Unit);
}

void UAlsasuaSquadManager::UnregisterUnit(AAlsasuaAIController* Unit)
{
    if (Unit) ActiveUnits.Remove(Unit);
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
    float Radius = 1500.f;
    float AngleStep = 360.f / ActiveUnits.Num();

    for (int32 i = 0; i < ActiveUnits.Num(); i++)
    {
        if (ActiveUnits[i].IsValid())
        {
            float Angle = FMath::DegreesToRadians(i * AngleStep);
            FVector TargetPos = PlayerLoc + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0);
            ActiveUnits[i]->MoveToLocation(TargetPos, 50.f);
        }
    }
}

void UAlsasuaSquadManager::ExecuteContainTactic()
{
    if (ActiveUnits.Num() == 0) return;

    // Create a perimeter around the center of mass of all units.
    FVector Center = FVector::ZeroVector;
    int32 ValidCount = 0;
    for (int32 i = 0; i < ActiveUnits.Num(); i++)
    {
        if (ActiveUnits[i].IsValid() && ActiveUnits[i]->GetPawn())
        {
            Center += ActiveUnits[i]->GetPawn()->GetActorLocation();
            ValidCount++;
        }
    }
    if (ValidCount > 0) Center /= ValidCount;

    float Radius = 2000.f;
    float AngleStep = 360.f / ActiveUnits.Num();

    for (int32 i = 0; i < ActiveUnits.Num(); i++)
    {
        if (ActiveUnits[i].IsValid())
        {
            float Angle = FMath::DegreesToRadians(i * AngleStep);
            FVector TargetPos = Center + FVector(FMath::Cos(Angle) * Radius, FMath::Sin(Angle) * Radius, 0);
            ActiveUnits[i]->MoveToLocation(TargetPos, 100.f);
        }
    }
}

void UAlsasuaSquadManager::ExecutePatrolTactic()
{
    // Each unit patrols a random nearby point.
    for (int32 i = 0; i < ActiveUnits.Num(); i++)
    {
        if (ActiveUnits[i].IsValid() && ActiveUnits[i]->GetPawn())
        {
            FVector CurrentLoc = ActiveUnits[i]->GetPawn()->GetActorLocation();
            FVector RandomOffset = FVector(FMath::RandRange(-3000.f, 3000.f), FMath::RandRange(-3000.f, 3000.f), 0);
            ActiveUnits[i]->MoveToLocation(CurrentLoc + RandomOffset, 200.f);
        }
    }
}

void UAlsasuaSquadManager::ExecuteSupportTactic()
{
    APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (!Player || ActiveUnits.Num() == 0) return;

    // All units converge toward the player's position from one side.
    FVector PlayerLoc = Player->GetActorLocation();
    FVector BaseDir = FVector(FMath::FRandRange(-1.f, 1.f), FMath::FRandRange(-1.f, 1.f), 0).GetSafeNormal();

    for (int32 i = 0; i < ActiveUnits.Num(); i++)
    {
        if (ActiveUnits[i].IsValid())
        {
            float Spread = 800.f;
            FVector Offset = BaseDir * 1000.f + FVector(FMath::RandRange(-Spread, Spread), FMath::RandRange(-Spread, Spread), 0);
            ActiveUnits[i]->MoveToLocation(PlayerLoc + Offset, 100.f);
        }
    }
}
