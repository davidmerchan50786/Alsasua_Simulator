#include "World/AlsasuaNightLightingSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Kismet/GameplayStatics.h"
#include "World/Time/TimeOfDayManager.h"

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

    // Las farolas las lleva UAlsasuaStreetLightController, uno por farola: si
    // este sistema también les escribía la intensidad, las dos escrituras se
    // pisaban a 0.5 s y 0.15 s y todo el alumbrado latía.
    UGameplayStatics::GetAllActorsOfClass(World, AStaticMeshActor::StaticClass(), Edificios);
    Edificios.RemoveAll([](AActor* A) {
        const FString L = A->GetName();
        return !L.Contains(TEXT("Edificio")) && !L.Contains(TEXT("Building")) && !L.Contains(TEXT("Tienda_"));
    });

    bEdificiosCached = true;
    UE_LOG(LogTemp, Log, TEXT("NightLighting: %d edificios con ventanas nocturnas"), Edificios.Num());
}

void UAlsasuaNightLightingSystem::UpdateNightFactor()
{
    UWorld* World = GetWorld();
    if (!World) return;

    // Mismo reloj que el sol y que el alumbrado. Antes salía de
    // GetServerWorldTimeSeconds(), un tiempo sin relación con la hora del
    // mundo: las ventanas se encendían con el sol en lo alto.
    const UTimeOfDayManager* TimeMgr = World->GetSubsystem<UTimeOfDayManager>();
    if (!TimeMgr) return;

    CurrentHour = TimeMgr->CurrentTime;

    // Rampa continua en el crepúsculo, envolviendo la medianoche.
    const float HoursIntoNight = FMath::Fmod(CurrentHour - SunsetHour + 24.f, 24.f);
    const float NightLength = FMath::Fmod(SunriseHour - SunsetHour + 24.f, 24.f);
    const float FadeSpan = FMath::Max(TransitionDuration, 0.01f);

    if (HoursIntoNight > NightLength)
    {
        NightFactor = 0.f;   // es de día
    }
    else
    {
        const float FadeIn = FMath::Clamp(HoursIntoNight / FadeSpan, 0.f, 1.f);
        const float FadeOut = FMath::Clamp((NightLength - HoursIntoNight) / FadeSpan, 0.f, 1.f);
        NightFactor = FMath::Min(FadeIn, FadeOut);
    }
}

void UAlsasuaNightLightingSystem::UpdateEdificios()
{
    for (AActor* Edificio : Edificios)
    {
        if (!Edificio) continue;

        // Un brillo fijo por edificio en vez de FRandRange en cada tick: cada
        // medio segundo las ventanas de todo el pueblo cambiaban de intensidad.
        const uint32 Seed = GetTypeHash(Edificio->GetFName());
        const float Bias = (float)(Seed & 0xFFFF) / 65535.f;
        const bool bIsComercio = Edificio->GetName().Contains(TEXT("Tienda"));

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
                if (!DynMat) continue;
                Mesh->SetMaterial(0, DynMat);
            }

            if (bIsComercio)
            {
                DynMat->SetVectorParameterValue(TEXT("EmissiveColor"), NeonColorComercio);
                DynMat->SetScalarParameterValue(TEXT("EmissivePower"), NightFactor * 3.0f);
            }
            else
            {
                DynMat->SetScalarParameterValue(TEXT("EmissivePower"),
                    NightFactor * FMath::Lerp(WindowEmissiveMin, WindowEmissiveMax, Bias));
            }
        }
    }
}

void UAlsasuaNightLightingSystem::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
    Super::TickComponent(DeltaTime, TickType, ThisTickFunction);

    if (!bEdificiosCached) CacheNightActors();

    UpdateNightFactor();
    UpdateEdificios();
}
