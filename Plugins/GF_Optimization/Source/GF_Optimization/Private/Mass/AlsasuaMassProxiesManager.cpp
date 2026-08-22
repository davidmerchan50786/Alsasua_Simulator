#include "Mass/AlsasuaMassProxiesManager.h"
#include "Mass/AlsasuaMassParallelManager.h"
#include "Engine/World.h"

void UAlsasuaMassProxies::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UAlsasuaMassProxies::Deinitialize()
{
    Super::Deinitialize();
}

void UAlsasuaMassProxies::Tick(float DeltaTime)
{
    // Delegar la lógica de actualización al ParallelManager.
    UWorld* W = GetWorld();
    if (!W) return;
    if (UAlsasuaMassParallelManager* ParallelMgr = W->GetSubsystem<UAlsasuaMassParallelManager>())
    {
        // Cull proxies demasiado lejos del jugador para ahorrar CPU.
        APlayerController* PC = W->GetFirstPlayerController();
        if (PC && PC->GetPawn())
        {
            const FVector PlayerLoc = PC->GetPawn()->GetActorLocation();
            const float CullDistSq = 30000.0f * 30000.0f; // 300m

            for (FMassProtesterProxy& P : ParallelMgr->Proxies)
            {
                if (FVector::DistSquared(P.Position, PlayerLoc) > CullDistSq)
                {
                    // Reposicionar lejos del jugador pero visible.
                    const float Angle = FMath::RandRange(0.f, 2.f * PI);
                    const float Dist = FMath::RandRange(20000.f, 28000.f);
                    P.Position = PlayerLoc + FVector(FMath::Cos(Angle) * Dist, FMath::Sin(Angle) * Dist, 0.f);
                    P.State = FMath::RandRange(0, 1);
                    P.Energy = FMath::RandRange(0.3f, 1.0f);
                }
            }
        }
    }
}

void UAlsasuaMassProxies::OnWantedLevelChanged(int32 NewLevel)
{
    // Cuando sube el nivel de búsqueda, los proxies reaccionan:
    //  - Level 0-1: ignoran al jugador
    //  - Level 2-3: huyen del jugador
    //  - Level 4-5: todos dispersan

    UWorld* W = GetWorld();
    if (!W) return;
    if (UAlsasuaMassParallelManager* ParallelMgr = W->GetSubsystem<UAlsasuaMassParallelManager>())
    {
        for (FMassProtesterProxy& P : ParallelMgr->Proxies)
        {
            if (NewLevel >= 4)
            {
                P.State = 1; // Walking (huyendo).
            }
            else if (NewLevel >= 2 && FMath::RandRange(0, 100) < NewLevel * 15)
            {
                P.State = 1; // Algunos huyen.
            }
        }
    }
}
