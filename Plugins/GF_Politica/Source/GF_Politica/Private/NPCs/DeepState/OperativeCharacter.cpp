#include "NPCs/DeepState/OperativeCharacter.h"
#include "AlsasuaTypes.h"
#include "Engine/OverlapResult.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

AOperativeCharacter::AOperativeCharacter()
{
    PrimaryActorTick.bCanEverTick = true;
}

void AOperativeCharacter::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (!bAmbushActive) return;

    AmbushTimer += DeltaTime;

    if (FMath::Fmod(AmbushTimer, 0.5f) < DeltaTime)
    {
        DamageNearbyTargets();
    }

    if (AmbushTimer >= AmbushDuration)
    {
        bAmbushActive = false;
        AmbushTimer = 0.f;
    }
}

void AOperativeCharacter::ExecuteAmbush()
{
    if (bAmbushActive) return;

    RevealOperative();
    AlertNearbyGuards();

    bAmbushActive = true;
    AmbushTimer = 0.f;

    UE_LOG(LogTemp, Warning, TEXT("Deep State: Emboscada ejecutada por %s"), *GetName());
}

void AOperativeCharacter::RevealOperative()
{
    bIsDisguised = false;

    if (USkeletalMeshComponent* SkelMesh = GetMesh())
    {
        SkelMesh->SetSimulatePhysics(true);
        SkelMesh->AddImpulse(FVector(0, 0, RevealImpulseForce), NAME_None, true);

        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [SkelMesh]()
        {
            if (SkelMesh)
            {
                SkelMesh->SetSimulatePhysics(false);
            }
        }, 0.3f, false);
    }
}

void AOperativeCharacter::DamageNearbyTargets()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<FOverlapResult> Overlaps;
    FCollisionShape Shape = FCollisionShape::MakeSphere(AmbushRadius);

    if (World->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity,
        ECC_Pawn, Shape))
    {
        for (const FOverlapResult& Hit : Overlaps)
        {
            AActor* Target = Hit.GetActor();
            if (!Target || Target == this) continue;

            if (IDamageable* Damageable = Cast<IDamageable>(Target))
            {
                Damageable->RecibirDano(
                    FMath::RoundToInt(AmbushDamagePerSecond * 0.5f),
                    GetActorLocation(),
                    ETipoDano::Impacto
                );
            }
        }
    }
}

void AOperativeCharacter::AlertNearbyGuards()
{
    OnAmbushAlert.Broadcast();
}
