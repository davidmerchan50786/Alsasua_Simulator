#include "World/AlsasuaWorldSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "Materials/Material.h"

static UMaterialInterface* BuscarMaterialFallbackTerreno()
{
    if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Terreno_Orto.M_Terreno_Orto")))
    {
        return Mat;
    }
    if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_TerrenoAlsasua.M_TerrenoAlsasua")))
    {
        return Mat;
    }
    return nullptr;
}

static void RepararMaterialTerrenoFallback(UWorld* World)
{
    if (!World) return;

    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscape::StaticClass(), LandscapeActors);

    UMaterialInterface* MaterialFallback = BuscarMaterialFallbackTerreno();
    if (!MaterialFallback)
    {
        MaterialFallback = UMaterial::GetDefaultMaterial(MD_Surface);
    }

    for (AActor* Actor : LandscapeActors)
    {
        if (!Actor) continue;

        ALandscape* LandscapeActor = Cast<ALandscape>(Actor);
        if (!LandscapeActor) continue;

        if (LandscapeActor->LandscapeMaterial != nullptr)
        {
            const FString MaterialName = LandscapeActor->LandscapeMaterial->GetName();
            if (MaterialName.Contains(TEXT("M_TerrenoAlsasua")) || MaterialName.Contains(TEXT("TerrenoAlsasua")))
            {
                LandscapeActor->LandscapeMaterial = MaterialFallback;
                LandscapeActor->MarkPackageDirty();
            }
        }
        else
        {
            LandscapeActor->LandscapeMaterial = MaterialFallback;
            LandscapeActor->MarkPackageDirty();
        }
    }
}

void UAlsasuaWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* World = GetWorld())
    {
        RepararMaterialTerrenoFallback(World);
    }
}

void UAlsasuaWorldSubsystem::SetGlobalWetness(float Wetness) {
    UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(
        nullptr, TEXT("/Game/Materials/MPC_AlsasuaGlobal.MPC_AlsasuaGlobal"));
    if (!MPC)
    {
        MPC = LoadObject<UMaterialParameterCollection>(
            nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
    }
    if (!MPC) return;

    UWorld* W = GetWorld();
    if (!W) return;

    UMaterialParameterCollectionInstance* Inst = W->GetParameterCollectionInstance(MPC);
    if (!Inst) return;

    float ClampedWetness = FMath::Clamp(Wetness, 0.f, 1.f);
    Inst->SetScalarParameterValue(FName("GlobalWetness"), ClampedWetness);
    Inst->SetScalarParameterValue(FName("RainIntensity"), ClampedWetness * 0.8f);
    Inst->SetScalarParameterValue(FName("PuddleOpacity"), FMath::Clamp(ClampedWetness * 1.2f, 0.f, 1.f));
}
