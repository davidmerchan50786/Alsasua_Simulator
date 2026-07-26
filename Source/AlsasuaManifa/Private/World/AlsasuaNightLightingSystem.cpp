#include "World/AlsasuaNightLightingSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/LightComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/SpotLightComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"

UAlsasuaNightLightingSystem::UAlsasuaNightLightingSystem()
{
    PrimaryComponentTick.bCanEverTick = true;
    PrimaryComponentTick.TickInterval = 0.5f;
}

void UAlsasuaNightLightingSystem::BeginPlay()
{
    Super::BeginPlay();
    CacheNightActors();
}

void UAlsasuaNightLightingSystem::CacheNightActors()
{
    UWorld* World = GetWorld();
    if (!World) return;

    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), Farolas);
    Farolas.RemoveAll([](AActor* A) {
        return !A->GetActorLabel().Contains(TEXT("Farola"));
    });

    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), Edificios);
    Edificios.RemoveAll([](AActor* A) {
        const FString L = A->GetActorLabel();
        return !L.Contains(TEXT("Edificio")) && !L.Contains(TEXT("Building")) && !L.Contains(TEXT("Tienda_"));
    });

    bFarolasCached = true;
    UE_LOG(LogTemp, Log, TEXT("NightLighting: %d farolas, %d edificios"), Farolas.Num(), Edificios.Num());
}

void UAlsasuaNightLightingSystem::UpdateNightFactor()
{
    UWorld* World = GetWorld();
    if (!World) return;

    const FDateTime Now = World->GetGameState()->GetServerWorldTimeSeconds();
    float TotalSeconds = World->GetGameState()->GetServerWorldTimeSeconds();
    CurrentHour = fmod(TotalSeconds / 3600.0f, 24.0f);

    float NightStart = SunsetHour;
    float NightEnd = SunriseHour + 24.0f;
    if (CurrentHour >= NightStart && CurrentHour <= 24.0f)
    {
        NightFactor = FMath::Clamp((CurrentHour - NightStart) / TransitionDuration, 0.0f, 1.0f);
    }
    else if (CurrentHour >= 0.0f && CurrentHour <= NightEnd - 24.0f)
    {
        float EndHour = NightEnd - 24.0f;
        NightFactor = FMath::Clamp(1.0f - (CurrentHour / (SunriseHour + TransitionDuration)), 0.0f, 1.0f);
    }
    else
    {
        NightFactor = 0.0f;
    }
}

void UAlsasuaNightLightingSystem::UpdateFarolas()
{
    for (AActor* Farola : Farolas)
    {
        if (!Farola) continue;

        TArray<UPointLightComponent*> PointLights;
        Farola->GetComponents<UPointLightComponent>(PointLights);
        for (UPointLightComponent* Light : PointLights)
        {
            Light->SetIntensity(NightFactor * FarolaIntensity);
            Light->SetLightColor(FarolaColor);
            Light->SetVisibility(NightFactor > 0.1f);
        }

        TArray<USpotLightComponent*> SpotLights;
        Farola->GetComponents<USpotLightComponent>(SpotLights);
        for (USpotLightComponent* Light : SpotLights)
        {
            Light->SetIntensity(NightFactor * FarolaIntensity);
            Light->SetLightColor(FarolaColor);
            Light->SetVisibility(NightFactor > 0.1f);
        }
    }
}

void UAlsasuaNightLightingSystem::UpdateEdificios()
{
    for (AActor* Edificio : Edificios)
    {
        if (!Edificio) continue;

        TArray<UStaticMeshComponent*> Meshes;
        Edificio->GetComponents<UStaticMeshComponent>(Meshes);
        for (UStaticMeshComponent* Mesh : Meshes)
        {
            UMaterialInterface* Mat = Mesh->GetMaterial(0);
            if (!Mat) continue;

            UMaterialInstanceDynamic* DynMat = Cast<UMaterialInstanceDynamic>(Mat);
            if (!DynMat)
            {
                DynMat = UMaterialInstanceDynamic::Create(Mat, this);
                if (DynMat) Mesh->SetMaterial(0, DynMat);
            }

            if (DynMat)
            {
                float EmissivePower = NightFactor * FMath::FRandRange(WindowEmissiveMin, WindowEmissiveMax);
                DynMat->SetScalarParameterValue(TEXT("EmissivePower"), EmissivePower);

                bool bIsComercio = Edificio->GetActorLabel().Contains(TEXT("Tienda"));
                if (bIsComercio)
                {
                    DynMat->SetVectorParameterValue(TEXT("EmissiveColor"), NeonColorComercio);
                    DynMat->SetScalarParameterValue(TEXT("EmissivePower"), NightFactor * 3.0f);
                }
            }
        }
    }
}

void UAlsasuaNightLightingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bFarolasCached) CacheNightActors();

    UpdateNightFactor();
    UpdateFarolas();
    UpdateEdificios();
}
