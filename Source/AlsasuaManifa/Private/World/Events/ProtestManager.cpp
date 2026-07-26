#include "World/Events/ProtestManager.h"
#include "Social/EvidenceSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"

AProtestManager::AProtestManager()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AProtestManager::BeginPlay()
{
    Super::BeginPlay();

    // Escuchar automáticamente al sistema de evidencias.
    UWorld* BW = GetWorld();
    if (!BW) return;
    if (UEvidenceSubsystem* ES = BW->GetSubsystem<UEvidenceSubsystem>())
    {
        ES->OnEvidencePublished.AddDynamic(this, &AProtestManager::TriggerProtestByEvidence);
    }
}

void AProtestManager::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    // Despawn stale protesters after their lifetime.
    for (int32 i = ActiveProtesters.Num() - 1; i >= 0; --i)
    {
        AActor* P = ActiveProtesters[i];
        if (!P) { ActiveProtesters.RemoveAt(i); continue; }

        if (P->GetGameTimeSinceCreation() > ProtesterLifetimeSeconds)
        {
            P->Destroy();
            ActiveProtesters.RemoveAt(i);
        }
    }
}

void AProtestManager::TriggerProtestByEvidence(FName EvidenceId)
{
    float Intensity = 1.0f;
    UWorld* TW = GetWorld();
    if (TW)
    {
        if (UEvidenceSubsystem* ES = TW->GetSubsystem<UEvidenceSubsystem>())
        {
            for (const FEvidenceItem& Ev : ES->CollectedEvidence)
            {
                if (Ev.EvidenceId == EvidenceId)
                {
                    Intensity = FMath::Clamp(Ev.ImpactPower / 10.f, 0.5f, 5.0f);
                    break;
                }
            }
        }
    }
    TriggerProtest(Intensity);
}

void AProtestManager::TriggerProtest(float Intensity)
{
    int32 Amount = FMath::Min(FMath::RoundToInt(BaseProtesterCount * Intensity), MaxProtesterCount);
    SpawnProtesters(Amount);

    UE_LOG(LogTemp, Warning, TEXT("MANIFESTACION INICIADA: %d ciudadanos en la calle (max %d)."),
        Amount, MaxProtesterCount);
}

void AProtestManager::SpawnProtesters(int32 Amount)
{
    if (!ProtesterClass || SpawnPoints.Num() == 0) return;

    UWorld* SW = GetWorld();
    if (!SW) return;

    for (int32 i = 0; i < Amount; ++i)
    {
        if (ActiveProtesters.Num() >= MaxProtesterCount) break;

        FVector Origin = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
        FVector FinalPos = Origin + FVector(FMath::RandRange(-800, 800), FMath::RandRange(-800, 800), 0);

        AActor* NewProtester = SW->SpawnActor<AActor>(ProtesterClass, FinalPos, FRotator::ZeroRotator);
        if (NewProtester)
        {
            ActiveProtesters.Add(NewProtester);
        }
    }
}

void AProtestManager::StopProtest()
{
    for (AActor* P : ActiveProtesters)
    {
        if (P) P->Destroy();
    }
    ActiveProtesters.Empty();
}
