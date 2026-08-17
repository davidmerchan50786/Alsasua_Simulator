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

    // Los tres LoadObject que había aquí se hacían al arrancar la partida,
    // pasara lo que pasara: tres cargas síncronas de assets que sólo hacen
    // falta si alguien llama a Spawn*, y de momento no llama nadie. En un clon
    // recién hecho —los Niagara los crea Tools/create_niagara_vfx.py, no
    // vienen en git— además soltaban tres avisos en el log del arranque.
    // Ahora se cargan la primera vez que se piden.
}

UNiagaraSystem* UAlsasuaVFXManager::Resolver(TObjectPtr<UNiagaraSystem>& Cache,
                                             const TCHAR* Ruta, bool& bIntentado)
{
    if (Cache) return Cache;
    if (bIntentado) return nullptr;   // no reintentar una ruta que no está
    bIntentado = true;
    Cache = LoadObject<UNiagaraSystem>(nullptr, Ruta);
    if (!Cache)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("VFXManager: no está %s. Se crea con Tools/create_niagara_vfx.py."), Ruta);
    }
    return Cache;
}

void UAlsasuaVFXManager::SpawnRainParticles(float Intensity)
{
    UNiagaraSystem* Rain = Resolver(RainSystem, TEXT("/Game/Effects/NS_Rain"), bRainIntentado);
    if (!Rain || Intensity < 0.05f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return;

    FVector Loc = PC->GetPawn()->GetActorLocation();

    if (!ActiveRain)
    {
        ActiveRain = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World, Rain, Loc, FRotator::ZeroRotator,
            FVector(1.0f), true, true);
    }

    if (ActiveRain)
    {
        ActiveRain->SetWorldLocation(Loc);
        ActiveRain->SetVariableFloat(FName("Intensity"), Intensity);
    }
}

void UAlsasuaVFXManager::SpawnLeafParticles(float WindSpeed)
{
    UNiagaraSystem* Leaf = Resolver(LeafSystem, TEXT("/Game/Effects/NS_Leaves"), bLeafIntentado);
    if (!Leaf || WindSpeed < 5.0f) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return;

    FVector Loc = PC->GetPawn()->GetActorLocation();

    if (!ActiveLeaf)
    {
        ActiveLeaf = UNiagaraFunctionLibrary::SpawnSystemAtLocation(
            World, Leaf, Loc, FRotator::ZeroRotator,
            FVector(1.0f), true, true);
    }

    if (ActiveLeaf)
    {
        ActiveLeaf->SetWorldLocation(Loc);
        ActiveLeaf->SetVariableFloat(FName("WindSpeed"), WindSpeed);
    }
}

void UAlsasuaVFXManager::SpawnDustParticles()
{
    UNiagaraSystem* Dust = Resolver(DustSystem, TEXT("/Game/Effects/NS_Dust"), bDustIntentado);
    if (!Dust) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = World->GetFirstPlayerController();
    if (!PC || !PC->GetPawn()) return;

    FVector Loc = PC->GetPawn()->GetActorLocation();
    UNiagaraFunctionLibrary::SpawnSystemAtLocation(
        World, Dust, Loc, FRotator::ZeroRotator,
        FVector(1.0f), true, true);
}

void UAlsasuaVFXManager::StopAllParticles()
{
    if (ActiveRain) { ActiveRain->Deactivate(); ActiveRain = nullptr; }
    if (ActiveLeaf) { ActiveLeaf->Deactivate(); ActiveLeaf = nullptr; }
}
