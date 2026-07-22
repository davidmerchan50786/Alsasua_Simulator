#include "Mision/SabotageMissionActor.h"
#include "Gameplay/Narco/NarcoSyndicateComponent.h"
#include "Politics/FactionSubsystem.h"
#include "Kismet/GameplayStatics.h"

ASabotageMissionActor::ASabotageMissionActor()
{
    PrimaryActorTick.bCanEverTick = false;
}

void ASabotageMissionActor::BeginPlay()
{
    Super::BeginPlay();
}

void ASabotageMissionActor::StartMission()
{
    SpawnEnemies();
    UE_LOG(LogTemp, Log, TEXT("Mision de sabotaje iniciada en: %s"), *TargetNodeId.ToString());
}

void ASabotageMissionActor::SpawnEnemies()
{
    if (!EnemyClass) return;

    for (int32 i = 0; i < EnemyCount; ++i)
    {
        FVector SpawnLoc = GetActorLocation() + FVector(FMath::RandRange(-500, 500), FMath::RandRange(-500, 500), 0);
        GetWorld()->SpawnActor<AActor>(EnemyClass, SpawnLoc, FRotator::ZeroRotator);
    }
}

void ASabotageMissionActor::OnTargetSabotaged(FName NodeId)
{
    if (NodeId == TargetNodeId)
    {
        // Recompensa política
        if (UFactionSubsystem* FS = GetWorld()->GetSubsystem<UFactionSubsystem>())
        {
            FS->RecordPoliticalEvent(FName("LaAsamblea"), FName("ElCentro"), PopularSupportReward);
        }

        OnMissionSuccess();
        UE_LOG(LogTemp, Warning, TEXT("OBJETIVO SABOTEADO. Mision completada."));
    }
}
