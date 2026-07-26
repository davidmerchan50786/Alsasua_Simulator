#include "Mass/AlsasuaMassParallelManager.h"
#include "Core/AlsasuaBudgetManager.h"
#include "Core/AlsasuaHitchProtector.h"
#include "Core/AlsasuaProfiling.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Async/ParallelFor.h"

void UAlsasuaMassParallelManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    SpawnProxies();
}

void UAlsasuaMassParallelManager::Tick(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_AlsasuaCrowd_MassParallelTick);
    UWorld* W = GetWorld();
    if (!W) return;
    UAlsasuaBudgetManager* Budget = W->GetSubsystem<UAlsasuaBudgetManager>();
    if (Budget && !Budget->CanExecute(EBudgetCategory::Simulation)) return;

    UAlsasuaHitchProtector* Hitch = W->GetSubsystem<UAlsasuaHitchProtector>();
    float LODScale = Hitch ? Hitch->GetGlobalLODScale() : 1.0f;

    UpdatePlayerCache();

    // Solo actualizar una fracción según LOD.
    const int32 TotalToUpdate = FMath::RoundToInt(Proxies.Num() * LODScale);

    if (TotalToUpdate >= 100)
    {
        // Usar procesamiento paralelo para grandes cantidades.
        ExecuteParallelUpdate(DeltaTime);
    }
    else
    {
        // Actualización secuencial para cantidades pequeñas.
        for (int32 i = 0; i < TotalToUpdate; ++i)
        {
            FMassProtesterProxy& P = Proxies[i];
            P.Position += P.Rotation.Vector() * (BaseSpeed * DeltaTime);
        }
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  ExecuteParallelUpdate: divide los proxies en batches y los procesa
//  en paralelo usando ParallelFor.
// ═══════════════════════════════════════════════════════════════════════════
void UAlsasuaMassParallelManager::ExecuteParallelUpdate(float DeltaTime)
{
    SCOPE_CYCLE_COUNTER(STAT_AlsasuaCrowd_ParallelFor);
    const int32 Num = Proxies.Num();
    if (Num == 0 || NumBatches <= 0) return;

    const int32 BatchSize = FMath::Max(1, Num / NumBatches);

    ParallelFor(NumBatches, [&](int32 BatchIndex)
    {
        const int32 Start = BatchIndex * BatchSize;
        const int32 End = (BatchIndex == NumBatches - 1) ? Num : FMath::Min(Start + BatchSize, Num);

        for (int32 i = Start; i < End; ++i)
        {
            FMassProtesterProxy& P = Proxies[i];

            // Movimiento base según estado.
            float Speed = BaseSpeed;
            switch (P.State)
            {
            case 0: Speed = 0.f; break;        // Idle: sin movimiento.
            case 1: Speed = BaseSpeed; break;   // Walking.
            case 2: Speed = BaseSpeed * 0.3f; break; // Chanting (casi quieto).
            default: break;
            }

            // Consumir energía al caminar.
            if (P.State == 1)
            {
                P.Energy = FMath::Max(0.f, P.Energy - 0.01f * DeltaTime);
                if (P.Energy <= 0.f)
                {
                    P.State = 0; // Cansado → idle.
                }
            }

            // Regenerar energía al estar idle.
            if (P.State == 0)
            {
                P.Energy = FMath::Min(1.f, P.Energy + 0.05f * DeltaTime);
            }

            P.Position += P.Rotation.Vector() * (Speed * DeltaTime);
        }
    });
}

// ═══════════════════════════════════════════════════════════════════════════
//  SpawnProxies: crea proxies distribuidos alrededor del jugador.
// ═══════════════════════════════════════════════════════════════════════════
void UAlsasuaMassParallelManager::SpawnProxies()
{
    Proxies.SetNum(TargetProxyCount);

    for (int32 i = 0; i < TargetProxyCount; ++i)
    {
        FMassProtesterProxy& P = Proxies[i];

        // Distribución en anillo alrededor del jugador.
        const float Angle = FMath::RandRange(0.f, 360.f);
        const float Dist = FMath::RandRange(SpawnRadius * 0.3f, SpawnRadius);
        const float Rad = FMath::DegreesToRadians(Angle);

        P.Position = CachedPlayerLocation + FVector(
            FMath::Cos(Rad) * Dist,
            FMath::Sin(Rad) * Dist,
            0.f
        );

        P.Rotation = FRotator(0.f, FMath::RandRange(0.f, 360.f), 0.f);
        P.State = FMath::RandRange(0, 2);
        P.Energy = FMath::RandRange(0.5f, 1.0f);
    }
}

// ═══════════════════════════════════════════════════════════════════════════
//  UpdatePlayerCache: cachea la posición del jugador cada 0.5s.
// ═══════════════════════════════════════════════════════════════════════════
void UAlsasuaMassParallelManager::UpdatePlayerCache()
{
    UWorld* W = GetWorld();
    if (!W) return;
    PlayerCacheTimer += W->GetDeltaSeconds();
    if (PlayerCacheTimer < 0.5f) return;
    PlayerCacheTimer = 0.f;

    if (APlayerController* PC = W->GetFirstPlayerController())
    {
        if (APawn* P = PC->GetPawn())
        {
            CachedPlayerLocation = P->GetActorLocation();
        }
    }
}
