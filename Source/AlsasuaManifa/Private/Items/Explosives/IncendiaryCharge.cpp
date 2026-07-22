#include "Items/Explosives/IncendiaryCharge.h"
#include "Components/SceneComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Vehicles/VehicleTarget.h"
#include "Kismet/GameplayStatics.h"
#include "TimerManager.h"

AIncendiaryCharge::AIncendiaryCharge()
{
    PrimaryActorTick.bCanEverTick = false;

    Root = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
    SetRootComponent(Root);

    MeshComp = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("MeshComp"));
    MeshComp->SetupAttachment(Root);
}

void AIncendiaryCharge::BeginPlay()
{
    Super::BeginPlay();
}

void AIncendiaryCharge::EndPlay(const EEndPlayReason::Type EndPlayReason)
{
    GetWorldTimerManager().ClearTimer(FuseTimerHandle);
    GetWorldTimerManager().ClearTimer(SlowMoHandle);
    Super::EndPlay(EndPlayReason);
}

void AIncendiaryCharge::ArmCharge(float FuseTime)
{
    bArmed = true;
    GetWorldTimerManager().SetTimer(FuseTimerHandle, this, &AIncendiaryCharge::Detonate, FuseTime, false);
}

void AIncendiaryCharge::Detonate()
{
    if (!bArmed)
    {
        return;
    }

    bArmed = false;
    GetWorldTimerManager().ClearTimer(FuseTimerHandle);
    Explode_Internal();
}

bool AIncendiaryCharge::AttachToAnchor(AActor* TargetActor, FName AnchorSocket)
{
    if (!TargetActor)
    {
        return false;
    }

    AttachToActor(TargetActor, FAttachmentTransformRules::SnapToTargetIncludingScale, AnchorSocket);
    return true;
}

void AIncendiaryCharge::Explode_Internal()
{
    if (!DeviceData)
    {
        return;
    }

    FVector Location = GetActorLocation();

    if (DeviceData->bEnableCinematicSlowMo)
    {
        UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 0.2f);
        GetWorldTimerManager().SetTimer(SlowMoHandle, [this]() {
            if (!IsValid(this)) return;
            UGameplayStatics::SetGlobalTimeDilation(GetWorld(), 1.0f);
        }, 0.5f, false);
    }

    TArray<AActor*> OverlappingVehicles;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), AVehicleTarget::StaticClass(), OverlappingVehicles);

    for (AActor* Actor : OverlappingVehicles)
    {
        AVehicleTarget* Vehicle = Cast<AVehicleTarget>(Actor);
        if (!Vehicle)
        {
            continue;
        }

        float Dist = FVector::Dist(Location, Vehicle->GetActorLocation());
        if (Dist <= DeviceData->BlastRadius)
        {
            Vehicle->ReceiveRadialImpulse(Location, DeviceData->ImpulseStrength, DeviceData->ChargePower);
            Vehicle->IgniteAtLocation(Location, DeviceData->FirePrefab, DeviceData->IgniteDuration);
        }
    }

    UGameplayStatics::SpawnEmitterAtLocation(GetWorld(), DeviceData->ExplosionFX, Location);
    UGameplayStatics::PlaySoundAtLocation(GetWorld(), DeviceData->ExplosionSound, Location);

    Destroy();
}
