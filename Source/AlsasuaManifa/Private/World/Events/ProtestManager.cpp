#include "World/Events/ProtestManager.h"
#include "Social/EvidenceSubsystem.h"
#include "Kismet/GameplayStatics.h"

AProtestManager::AProtestManager()
{
    PrimaryActorTick.bCanEverTick = false;
}

void AProtestManager::BeginPlay()
{
    Super::BeginPlay();

    // Escuchar automáticamente al sistema de evidencias
    if (UEvidenceSubsystem* ES = GetWorld()->GetSubsystem<UEvidenceSubsystem>())
    {
        ES->OnEvidencePublished.AddDynamic(this, &AProtestManager::TriggerProtestByEvidence);
    }
}

// Wrapper para delegados (recibe EvidenceId pero dispara con intensidad fija por ahora)
void AProtestManager::TriggerProtestByEvidence(FName EvidenceId)
{
    TriggerProtest(1.5f); // Intensidad aumentada por filtración
}

void AProtestManager::TriggerProtest(float Intensity)
{
    int32 Amount = FMath::RoundToInt(BaseProtesterCount * Intensity);
    SpawnProtesters(Amount);

    UE_LOG(LogTemp, Warning, TEXT("MANIFESTACION INICIADA: %d ciudadanos en la calle."), Amount);
}

void AProtestManager::SpawnProtesters(int32 Amount)
{
    if (!ProtesterClass || SpawnPoints.Num() == 0) return;

    for (int32 i = 0; i < Amount; ++i)
    {
        FVector Origin = SpawnPoints[FMath::RandRange(0, SpawnPoints.Num() - 1)];
        FVector FinalPos = Origin + FVector(FMath::RandRange(-800, 800), FMath::RandRange(-800, 800), 0);

        AActor* NewProtester = GetWorld()->SpawnActor<AActor>(ProtesterClass, FinalPos, FRotator::ZeroRotator);
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
