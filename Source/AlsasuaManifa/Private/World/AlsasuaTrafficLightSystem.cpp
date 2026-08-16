#include "World/AlsasuaTrafficLightSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UAlsasuaTrafficLightSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaTrafficLightSystem::ColocarSemaforos()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Semaforos.Empty();

    TArray<FVector> JunctionPoints;
    TArray<TSharedPtr<FJsonValue>> RoadsArr;
    if (JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), RoadsArr, { TEXT("roads") }))
    {
        for (const auto& RV : RoadsArr)
        {
            const TSharedPtr<FJsonObject>& Road = RV->AsObject();
            if (!Road) continue;
            const TArray<TSharedPtr<FJsonValue>>* PtsArr;
            if (!Road->TryGetArrayField(TEXT("points"), PtsArr)) continue;
            if (PtsArr->Num() < 2) continue;

            for (const auto& PV : *PtsArr)
            {
                const TSharedPtr<FJsonObject>& PO = PV->AsObject();
                if (!PO) continue;
                JunctionPoints.Add(UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(),
                    FVector(PO->GetNumberField(TEXT("x")), 0.0f, PO->GetNumberField(TEXT("z")))));
            }
        }
    }

    TArray<FVector> Candidates;
    const float MinDist = 1500.0f;
    for (const FVector& Pt : JunctionPoints)
    {
        int32 NearbyCount = 0;
        for (const FVector& Other : JunctionPoints)
        {
            if (FVector::DistSquared(Pt, Other) < MinDist * MinDist)
                NearbyCount++;
        }
        if (NearbyCount >= 6)
        {
            bool bTooClose = false;
            for (const FVector& C : Candidates)
            {
                if (FVector::DistSquared(Pt, C) < 800.0f * 800.0f)
                {
                    bTooClose = true;
                    break;
                }
            }
            if (!bTooClose) Candidates.Add(Pt);
        }
    }

    int32 Placed = 0;
    for (int32 i = 0; i < MaxSemaforos && i < Candidates.Num(); i++)
    {
        const FVector& Pos = Candidates[i];

        FTrafficLight Light;
        Light.Posicion = Pos;
        Light.Rotacion = 0.0f;
        Light.Calle = FString::Printf(TEXT("Interseccion_%d"), i);
        Light.Barrio = TEXT("");
        Light.bActivo = true;

        float PostHeight = 350.0f;

        AStaticMeshActor* PostActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (PostActor)
        {
            PostActor->SetMobility(EComponentMobility::Static);
            PostActor->SetActorScale3D(FVector(0.08f, 0.08f, PostHeight / 100.0f));

            UStaticMesh* PoleMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/CitySample/Prop/Kit_StreetLamp_A/Mesh/SM_StreetLamp_A_TrafficLight_Pole"));
            if (PoleMesh)
                PostActor->GetStaticMeshComponent()->SetStaticMesh(PoleMesh);
        }

        FVector LightPos = Pos;
        LightPos.Z += PostHeight;

        AStaticMeshActor* BoxActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), LightPos, FRotator::ZeroRotator);
        if (BoxActor)
        {
            BoxActor->SetMobility(EComponentMobility::Static);
            BoxActor->SetActorScale3D(FVector(0.25f, 0.25f, 0.7f));

            UStaticMesh* StopLightMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/CitySample/Prop/Kit_StreetLamp_A/Mesh/SM_StreetLamp_A_StopLight_A"));
            if (StopLightMesh)
                BoxActor->GetStaticMeshComponent()->SetStaticMesh(StopLightMesh);
        }

        // Una sola luz por semáforo, no tres. Antes se creaba una APointLight
        // por LED y se encendían las tres a la vez: rojo, ámbar y verde
        // simultáneos, 36 luces para 12 cruces. El color y la intensidad los
        // manda ahora la fase del ciclo.
        FVector LedPos = LightPos;
        LedPos.Z -= 25.0f;
        if (APointLight* Led = World->SpawnActor<APointLight>(
                APointLight::StaticClass(), LedPos, FRotator::ZeroRotator))
        {
            if (UPointLightComponent* PL = Cast<UPointLightComponent>(Led->GetLightComponent()))
            {
                PL->SetAttenuationRadius(200.0f);
                PL->SetSourceRadius(5.0f);
            }
            Light.Luz = Led;
        }

        // Desfase repartido por el ciclo entero: doce cruces cambiando a la vez
        // no pasa en ningún pueblo.
        const float Ciclo = SegVerde + SegAmbar + SegRojo;
        Light.Desfase = Ciclo * (float)i / (float)FMath::Max(1, MaxSemaforos);

        Semaforos.Add(Light);
        Placed++;
    }

    // Fase inicial ya puesta: si no, hasta el primer tick todos salen apagados.
    Aplicar();

    UE_LOG(LogTemp, Log, TEXT("TrafficLights: %d semáforos en intersecciones reales (%d candidatos)"),
        Placed, Candidates.Num());
    return Placed;
}

EFaseSemaforo UAlsasuaTrafficLightSystem::FaseEn(float TiempoCiclo) const
{
    if (TiempoCiclo < SegVerde)            return EFaseSemaforo::Verde;
    if (TiempoCiclo < SegVerde + SegAmbar) return EFaseSemaforo::Ambar;
    return EFaseSemaforo::Rojo;
}

void UAlsasuaTrafficLightSystem::Aplicar()
{
    const float Ciclo = FMath::Max(1.f, SegVerde + SegAmbar + SegRojo);
    for (FTrafficLight& S : Semaforos)
    {
        if (!S.bActivo || !S.Luz) continue;

        S.Fase = FaseEn(FMath::Fmod(Reloj + S.Desfase, Ciclo));

        FLinearColor Color;
        float Intensidad;
        switch (S.Fase)
        {
        case EFaseSemaforo::Verde: Color = FLinearColor(0.0f, 0.7f, 0.1f); Intensidad = 1200.f; break;
        case EFaseSemaforo::Ambar: Color = FLinearColor(0.9f, 0.55f, 0.0f); Intensidad = 1600.f; break;
        default:                   Color = FLinearColor(0.9f, 0.05f, 0.0f); Intensidad = 2000.f; break;
        }

        if (UPointLightComponent* PL = Cast<UPointLightComponent>(S.Luz->GetLightComponent()))
        {
            PL->SetLightColor(Color);
            PL->SetIntensity(Intensidad);
        }
    }
}

void UAlsasuaTrafficLightSystem::Tick(float DeltaTime)
{
    if (Semaforos.Num() == 0) return;
    Reloj += DeltaTime;

    // Los setters de luz invalidan el draw-cache de la escena, así que van a
    // 4 Hz como el sol, el skylight y la niebla (regla 2 del RESUMEN_TECNICO).
    // Un semáforo no necesita más: la fase más corta dura tres segundos.
    DesdeUltimoRefresco += DeltaTime;
    if (DesdeUltimoRefresco < 0.25f) return;
    DesdeUltimoRefresco = 0.f;

    Aplicar();
}

void UAlsasuaTrafficLightSystem::Deinitialize()
{
    Semaforos.Empty();
    Super::Deinitialize();
}
