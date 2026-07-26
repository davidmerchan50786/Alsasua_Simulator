#include "NPCs/DeepState/OperativeCharacter.h"
#include "AlsasuaTypes.h"
#include "Character/Stealth/GuardDetectionComponent.h"
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

    // Daño continuo a objetivos cercanos durante la emboscada.
    if (FMath::Fmod(AmbushTimer, 0.5f) < DeltaTime)
    {
        DamageNearbyTargets();
    }

    // Terminar emboscada tras la duración.
    if (AmbushTimer >= AmbushDuration)
    {
        bAmbushActive = false;
        AmbushTimer = 0.f;
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  ExecuteAmbush: revela al operativo y ejecuta la emboscada.
// ═══════════════════════════════════════════════════════════════════════════
void AOperativeCharacter::ExecuteAmbush()
{
    if (bAmbushActive) return;

    RevealOperative();
    AlertNearbyGuards();

    bAmbushActive = true;
    AmbushTimer = 0.f;

    UE_LOG(LogTemp, Warning, TEXT("Deep State: Emboscada ejecutada por %s"), *GetName());
}

// ═══════════════════════════════════════════════════════════════════════════
//  RevealOperative: elimina el disfraz y aplica impulso visual.
// ═══════════════════════════════════════════════════════════════════════════
void AOperativeCharacter::RevealOperative()
{
    bIsDisguised = false;

    // Impulso visual al revelarse (el operativo se "transforma").
    if (USkeletalMeshComponent* Mesh = GetMesh())
    {
        Mesh->SetSimulatePhysics(true);
        Mesh->AddImpulse(FVector(0, 0, RevealImpulseForce), NAME_None, true);

        // Desactivar física tras un breve momento.
        FTimerHandle TimerHandle;
        GetWorldTimerManager().SetTimer(TimerHandle, [Mesh]()
        {
            if (Mesh)
            {
                Mesh->SetSimulatePhysics(false);
            }
        }, 0.3f, false);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  DamageNearbyTargets: daño por radio a enemigos cercanos.
// ═══════════════════════════════════════════════════════════════════════════
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

            // Aplicar daño si es un personaje con sistema de vida.
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

// ═══════════════════════════════════════════════════════════════════════════
//  AlertNearbyGuards: notifica a guardias cercanos.
// ═══════════════════════════════════════════════════════════════════════════
void AOperativeCharacter::AlertNearbyGuards()
{
    UWorld* World = GetWorld();
    if (!World) return;

    TArray<FOverlapResult> Overlaps;
    FCollisionShape Shape = FCollisionShape::MakeSphere(AmbushRadius * 1.5f);

    if (World->OverlapMultiByChannel(Overlaps, GetActorLocation(), FQuat::Identity,
        ECC_Pawn, Shape))
    {
        for (const FOverlapResult& Hit : Overlaps)
        {
            AActor* Target = Hit.GetActor();
            if (!Target || Target == this) continue;

            if (UGuardDetectionComponent* Detection = Target->FindComponentByClass<UGuardDetectionComponent>())
            {
                Detection->ForceAlertState(EGuardAlertState::Combat);
            }
        }
    }
}
