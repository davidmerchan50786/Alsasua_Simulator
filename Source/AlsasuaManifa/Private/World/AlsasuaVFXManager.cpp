#include "World/AlsasuaVFXManager.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraSystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"

void UAlsasuaVFXManager::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    RainSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Effects/NS_Rain"));
    LeafSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Effects/NS_Leaves"));
    DustSystem = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/Effects/NS_Dust"));
}

void UAlsasuaVFXManager::SpawnRainParticles(float Intensity)
{
    if (!RainSystem || Intensity < 0.05f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return;

    FVector Loc = PC->GetPawn()->GetActorLocation();

    if (!ActiveRain)
    {
        ActiveRain = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World, RainSystem, Loc, FRotator::ZeroRotator,
            FVector(1.0f), true, true, true);
    }

    if (ActiveRain)
    {
        ActiveRain->SetWorldLocation(Loc);
        ActiveRain->SetVariableFloat(FName("Intensity"), Intensity);
    }
}

void UAlsasuaVFXManager::SpawnLeafParticles(float WindSpeed)
{
    if (!LeafSystem || WindSpeed < 5.0f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return;

    FVector Loc = PC->GetPawn()->GetActorLocation();

    if (!ActiveLeaf)
    {
        ActiveLeaf = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World, LeafSystem, Loc, FRotator::ZeroRotator,
            FVector(1.0f), true, true, true);
    }

    if (ActiveLeaf)
    {
        ActiveLeaf->SetWorldLocation(Loc);
        ActiveLeaf->SetVariableFloat(FName("WindSpeed"), WindSpeed);
    }
}

void UAlsasuaVFXManager::SpawnDustParticles()
{
    if (!DustSystem) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return;

    FVector Loc = PC->GetPawn()->GetActorLocation();
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World, DustSystem, Loc, FRotator::ZeroRotator,
        FVector(1.0f), true, true, true);
}

void UAlsasuaVFXManager::StopAllParticles()
{
    if (ActiveRain) { ActiveRain->Deactivate(); ActiveRain = nullptr; }
    if (ActiveLeaf) { ActiveLeaf->Deactivate(); ActiveLeaf = nullptr; }
}
