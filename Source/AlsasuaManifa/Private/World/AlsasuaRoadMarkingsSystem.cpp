#include "World/AlsasuaRoadMarkingsSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaRoadMarkingsSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaRoadMarkingsSystem::GenerarMarcas()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath)) return 0;

    TSharedPtr<FJsonValue> RootVal;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RootVal) || !RootVal.IsValid()) return 0;

    const TArray<TSharedPtr<FJsonValue>>* RoadsArr;
    if (!RootVal->TryGetArray(RoadsArr)) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    Marcas.Empty();
    int32 TotalCruces = 0;
    int32 TotalLineas = 0;
    int32 TotalStop = 0;

    for (const auto& RoadVal : *RoadsArr)
    {
        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        const FString Type = Road->HasField(TEXT("type")) ? Road->GetStringField(TEXT("type")) : TEXT("");
        const FString Calle = Road->HasField(TEXT("name")) ? Road->GetStringField(TEXT("name")) : TEXT("");
        const FString Barrio = Road->HasField(TEXT("barrio")) ? Road->GetStringField(TEXT("barrio")) : TEXT("");
        const float RoadWidth = Road->HasField(TEXT("width")) ? Road->GetNumberField(TEXT("width")) : 6.0f;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() < 2) continue;

        for (int32 i = 0; i < PointsArr->Num() - 1; i++)
        {
            const TSharedPtr<FJsonObject>& P0 = (*PointsArr)[i]->AsObject();
            const TSharedPtr<FJsonObject>& P1 = (*PointsArr)[i + 1]->AsObject();
            if (!P0 || !P1) continue;

            FVector Loc0 = UAlsasuaGeoData::RelLocalToUE5(FVector(
                P0->GetNumberField(TEXT("x")), 0.0f, P0->GetNumberField(TEXT("z"))));
            FVector Loc1 = UAlsasuaGeoData::RelLocalToUE5(FVector(
                P1->GetNumberField(TEXT("x")), 0.0f, P1->GetNumberField(TEXT("z"))));

            FVector Centro = (Loc0 + Loc1) * 0.5f;
            FVector Direccion = (Loc1 - Loc0).GetSafeNormal();
            FVector Normal = FVector(-Direccion.Y, Direccion.X, 0);
            float Largo = FVector::Distance(Loc0, Loc1);
            float Angle = FMath::RadiansToDegrees(FMath::Atan2(Direccion.Y, Direccion.X));

            if (RoadWidth >= 6.0f && TotalLineas < MaxLineasCentrales && Largo > 300.0f)
            {
                FRoadMarking LineaCentral;
                LineaCentral.Tipo = TEXT("linea_central");
                LineaCentral.Posicion = Centro;
                LineaCentral.Posicion.Z += 2.0f;
                LineaCentral.Rotacion = Angle;
                LineaCentral.Ancho = 10.0f;
                LineaCentral.Largo = Largo;
                LineaCentral.Calle = Calle;
                LineaCentral.Barrio = Barrio;

                AStaticMeshActor* LineaActor = World->SpawnActor<AStaticMeshActor>(
                    AStaticMeshActor::StaticClass(), LineaCentral.Posicion, FRotator(0, Angle, 0));
                if (LineaActor)
                {
                    LineaActor->SetMobility(EComponentMobility::Static);
                    LineaActor->SetActorScale3D(FVector(Largo / 100.0f, 0.1f, 0.02f));

                    UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                        TEXT("/Game/EngineBasicShapes/Plane"));
                    if (PlaneMesh)
                        LineaActor->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

                    UMaterialInterface* WhiteMat = LoadObject<UMaterialInterface>(nullptr,
                        TEXT("/Game/Materiales/M_Marca_Blanca"));
                    if (WhiteMat)
                        LineaActor->GetStaticMeshComponent()->SetMaterial(0, WhiteMat);

#if WITH_EDITOR
                    LineaActor->SetActorLabel(*FString::Printf(TEXT("Linea_%s_%d"), *Calle.Left(8), TotalLineas));
#endif
                }

                Marcas.Add(LineaCentral);
                TotalLineas++;
            }

            if (i == 0 && TotalCruces < MaxCrucesPeatonales)
            {
                FRoadMarking Cruce;
                Cruce.Tipo = TEXT("cruce_peatonal");
                Cruce.Posicion = Loc0;
                Cruce.Posicion.Z += 2.0f;
                Cruce.Rotacion = Angle;
                Cruce.Ancho = RoadWidth * 100.0f;
                Cruce.Largo = 300.0f;
                Cruce.Calle = Calle;
                Cruce.Barrio = Barrio;

                for (int32 s = 0; s < 5; s++)
                {
                    FVector StripePos = Loc0 + Normal * (Cruce.Ancho * 0.5f - s * Cruce.Ancho / 5.0f);
                    StripePos.Z += 2.0f;

                    AStaticMeshActor* Stripe = World->SpawnActor<AStaticMeshActor>(
                        AStaticMeshActor::StaticClass(), StripePos, FRotator(0, Angle, 0));
                    if (Stripe)
                    {
                        Stripe->SetMobility(EComponentMobility::Static);
                        Stripe->SetActorScale3D(FVector(3.0f, 0.3f, 0.02f));

                        UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                            TEXT("/Game/EngineBasicShapes/Plane"));
                        if (PlaneMesh)
                            Stripe->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

                        UMaterialInterface* WhiteMat = LoadObject<UMaterialInterface>(nullptr,
                            TEXT("/Game/Materiales/M_Marca_Blanca"));
                        if (WhiteMat)
                            Stripe->GetStaticMeshComponent()->SetMaterial(0, WhiteMat);

#if WITH_EDITOR
                        Stripe->SetActorLabel(*FString::Printf(TEXT("Cruce_%s_%d_%d"),
                            *Calle.Left(8), TotalCruces, s));
#endif
                    }
                }

                Marcas.Add(Cruce);
                TotalCruces++;
            }

            if (Type == TEXT("residential") && i == PointsArr->Num() - 2 && TotalStop < MaxLineasStop)
            {
                FRoadMarking Stop;
                Stop.Tipo = TEXT("linea_stop");
                Stop.Posicion = Loc1;
                Stop.Posicion.Z += 2.0f;
                Stop.Rotacion = Angle;
                Stop.Ancho = RoadWidth * 80.0f;
                Stop.Largo = 20.0f;
                Stop.Calle = Calle;
                Stop.Barrio = Barrio;

                AStaticMeshActor* StopLine = World->SpawnActor<AStaticMeshActor>(
                    AStaticMeshActor::StaticClass(), Stop.Posicion, FRotator(0, Angle, 0));
                if (StopLine)
                {
                    StopLine->SetMobility(EComponentMobility::Static);
                    StopLine->SetActorScale3D(FVector(2.0f, RoadWidth * 0.8f, 0.02f));

                    UStaticMesh* PlaneMesh = LoadObject<UStaticMesh>(nullptr,
                        TEXT("/Game/EngineBasicShapes/Plane"));
                    if (PlaneMesh)
                        StopLine->GetStaticMeshComponent()->SetStaticMesh(PlaneMesh);

                    UMaterialInterface* WhiteMat = LoadObject<UMaterialInterface>(nullptr,
                        TEXT("/Game/Materiales/M_Marca_Blanca"));
                    if (WhiteMat)
                        StopLine->GetStaticMeshComponent()->SetMaterial(0, WhiteMat);

#if WITH_EDITOR
                    StopLine->SetActorLabel(*FString::Printf(TEXT("Stop_%s_%d"), *Calle.Left(8), TotalStop));
#endif
                }

                Marcas.Add(Stop);
                TotalStop++;
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("RoadMarkings: %d cruces, %d líneas centrales, %d stop"),
        TotalCruces, TotalLineas, TotalStop);
    return Marcas.Num();
}
