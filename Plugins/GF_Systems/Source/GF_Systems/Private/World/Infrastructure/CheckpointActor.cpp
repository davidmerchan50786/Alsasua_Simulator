#include "World/Infrastructure/CheckpointActor.h"
#include "Components/BoxComponent.h"
#include "Politics/StateOfAlarmSubsystem.h"

ACheckpointActor::ACheckpointActor()
{
    PrimaryActorTick.bCanEverTick = false;

    RootComp = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    RootComponent = RootComp;

    TriggerZone = CreateDefaultSubobject<UBoxComponent>(TEXT("TriggerZone"));
    TriggerZone->SetupAttachment(RootComponent);
    TriggerZone->SetBoxExtent(FVector(500.f, 1000.f, 200.f));
}

void ACheckpointActor::BeginPlay()
{
    Super::BeginPlay();

    UWorld* W = GetWorld();
    if (W)
    {
        if (UStateOfAlarmSubsystem* SAS = W->GetSubsystem<UStateOfAlarmSubsystem>())
        {
            SAS->OnAlarmLevelChanged.AddDynamic(this, &ACheckpointActor::HandleAlarmChanged);
            HandleAlarmChanged(SAS->CurrentLevel);
        }
    }
}

void ACheckpointActor::HandleAlarmChanged(EStateOfAlarmLevel NewLevel)
{
    bool bShouldBeActive = (uint8)NewLevel >= (uint8)RequiredLevel;
    SetCheckpointActive(bShouldBeActive);
}

void ACheckpointActor::SetCheckpointActive(bool bActive)
{
    // Activar/Desactivar collision y visibilidad (en BP se puede extender para efectos)
    SetActorHiddenInGame(!bActive);
    SetActorEnableCollision(bActive);

    if (bActive && SpawnedGuards.Num() == 0 && GuardClass)
    {
        for (int32 i = 0; i < MaxGuards; i++)
        {
            FVector SpawnLoc = GetActorLocation() + GetActorRightVector() * (i * 200.f - 200.f);
            UWorld* W = GetWorld();
            if (!W) continue;
            AActor* Guard = W->SpawnActor<AActor>(GuardClass, SpawnLoc, GetActorRotation());
            if (Guard) SpawnedGuards.Add(Guard);
        }
    }
    else if (!bActive)
    {
        for (AActor* G : SpawnedGuards) { if (G) G->Destroy(); }
        SpawnedGuards.Empty();
    }
}
