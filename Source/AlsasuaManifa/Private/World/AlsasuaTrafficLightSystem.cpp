#include "World/AlsasuaTrafficLightSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/PointLightComponent.h"
#include "Engine/PointLight.h"
#include "GeoDataAlsasua.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UAlsasuaTrafficLightSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UAlsasuaTrafficLightSystem::Deinitialize()
{
    Semaforos.Empty();
    Reloj = 0.f;
    Super::Deinitialize();
}

void UAlsasuaTrafficLightSystem::Tick(float DeltaTime)
{
    Reloj += DeltaTime;
    DesdeUltimoRefresco += DeltaTime;
    if (DesdeUltimoRefresco < 0.25f) return;
    DesdeUltimoRefresco = 0.f;
    Aplicar();
}

EFaseSemaforo UAlsasuaTrafficLightSystem::FaseEn(float TiempoCiclo) const
{
    const float Periodo = SegVerde + SegAmbar + SegRojo;
    const float T = FMath::Fmod(TiempoCiclo + 1000.f * Periodo, Periodo);
    if (T < SegVerde) return EFaseSemaforo::Verde;
    if (T < SegVerde + SegAmbar) return EFaseSemaforo::Ambar;
    return EFaseSemaforo::Rojo;
}

void UAlsasuaTrafficLightSystem::Aplicar()
{
    const float Periodo = SegVerde + SegAmbar + SegRojo;
    for (FTrafficLight& L : Semaforos)
    {
        if (!L.bActivo || !L.Luz) continue;
        L.Fase = FaseEn(Reloj + L.Desfase);

        UPointLightComponent* PL = Cast<UPointLightComponent>(L.Luz->GetLightComponent());
        if (!PL) continue;

        switch (L.Fase)
        {
        case EFaseSemaforo::Verde:
            PL->SetLightColor(FLinearColor(0.0f, 0.7f, 0.0f));
            PL->SetIntensity(2000.f);
            break;
        case EFaseSemaforo::Ambar:
            PL->SetLightColor(FLinearColor(0.8f, 0.6f, 0.0f));
            PL->SetIntensity(1500.f);
            break;
        case EFaseSemaforo::Rojo:
            PL->SetLightColor(FLinearColor(0.8f, 0.0f, 0.0f));
            PL->SetIntensity(2000.f);
            break;
        }
    }
}

int32 UAlsasuaTrafficLightSystem::ColocarSemaforos()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Semaforos.Empty();

    TArray<FVector> JunctionPoints;
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    TArray<FString> Lines;
    if (FFileHelper::LoadFileToStringArray(Lines, *JsonPath))
    {
        FString Js;
        for (const FString& L : Lines) Js += L;

        TArray<TSharedPtr<FJsonValue>> RoadsArr;
        TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Js);
        if (FJsonSerializer::Deserialize(Rd, RoadsArr))
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
            if (!PoleMesh)
                PoleMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
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
            if (!StopLightMesh)
                StopLightMesh = LoadObject<UStaticMesh>(nullptr,
                    TEXT("/Engine/BasicShapes/Cube.Cube"));
            if (StopLightMesh)
                BoxActor->GetStaticMeshComponent()->SetStaticMesh(StopLightMesh);
        }

        for (int32 L = 0; L < 3; L++)
        {
            FVector LedPos = LightPos;
            LedPos.Z -= (L * 25.0f);

            APointLight* Led = World->SpawnActor<APointLight>(
                APointLight::StaticClass(), LedPos, FRotator::ZeroRotator);
            if (Led)
            {
                FLinearColor LedColor;
                if (L == 0) LedColor = FLinearColor(0.8f, 0.0f, 0.0f);
                else if (L == 1) LedColor = FLinearColor(0.8f, 0.6f, 0.0f);
                else LedColor = FLinearColor(0.0f, 0.7f, 0.0f);

			UPointLightComponent* PointLightComp = Cast<UPointLightComponent>(Led->GetLightComponent());
			if (PointLightComp)
			{
				PointLightComp->SetIntensity(L == 0 ? 2000.0f : 200.0f);
				PointLightComp->SetLightColor(LedColor);
				PointLightComp->SetAttenuationRadius(200.0f);
				PointLightComp->SetSourceRadius(5.0f);
			}
            }
        }

        Semaforos.Add(Light);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficLights: %d semaforos en intersecciones reales (%d candidatos)"),
        Placed, Candidates.Num());
    return Placed;
}
