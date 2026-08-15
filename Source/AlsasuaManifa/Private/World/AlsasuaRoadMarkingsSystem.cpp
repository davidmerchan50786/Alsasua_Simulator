#include "World/AlsasuaRoadMarkingsSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "CargarMaterialComun.h"
#include "HAL/ConsoleManager.h"

#include "Materials/MaterialInterface.h"

static TAutoConsoleVariable<int32> CVarSkipRoadMarkings(
    TEXT("alsasua.SkipRoadMarkings"),
    0,
    TEXT("Skips road-marking generation for profiling"),
    ECVF_Cheat);

static UMaterialInterface* CargarMaterialMarcas()
{
	if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Marca_Blanca.M_Marca_Blanca")))
		return M;
	return CargarMaterialConFallback(
		TEXT("/Game/Road/Material/MI/M_Asphalt_Master_Inst_Crosswalk.M_Asphalt_Master_Inst_Crosswalk"),
		TEXT("/Game/Materiales/M_Terreno_Calles.M_Terreno_Calles"),
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
}

void UAlsasuaRoadMarkingsSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaRoadMarkingsSystem::GenerarMarcas()
{
    if (CVarSkipRoadMarkings.GetValueOnAnyThread() != 0)
    {
        UE_LOG(LogTemp, Log, TEXT("RoadMarkings skipped by alsasua.SkipRoadMarkings"));
        return 0;
    }

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

    UInstancedStaticMeshComponent* LineasISM = nullptr;
    UInstancedStaticMeshComponent* CrucesISM = nullptr;
    UInstancedStaticMeshComponent* StopISM = nullptr;

    auto CrearISM = [&](const TCHAR* Name, UInstancedStaticMeshComponent*& OutComp)
    {
        if (OutComp) return;
        AActor* Holder = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        if (!Holder) return;
        Holder->Rename(Name);
        Holder->SetHidden(true);
        OutComp = NewObject<UInstancedStaticMeshComponent>(Holder, UInstancedStaticMeshComponent::StaticClass(), FName(Name));
        if (!OutComp) return;
        OutComp->RegisterComponentWithWorld(World);
        OutComp->SetMobility(EComponentMobility::Static);
        OutComp->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        OutComp->SetCanEverAffectNavigation(false);
        OutComp->CastShadow = true;
        OutComp->SetStaticMesh(LoadObject<UStaticMesh>(nullptr, TEXT("/Engine/BasicShapes/Plane.Plane")));
        UMaterialInterface* WhiteMat = CargarMaterialMarcas();
        if (WhiteMat) OutComp->SetMaterial(0, WhiteMat);
        OutComp->SetFlags(RF_Transactional);
        Holder->AddInstanceComponent(OutComp);
        Holder->SetRootComponent(OutComp);
    };

    CrearISM(TEXT("RoadMarking_Lines"), LineasISM);
    CrearISM(TEXT("RoadMarking_Crosswalks"), CrucesISM);
    CrearISM(TEXT("RoadMarking_Stop"), StopISM);

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

            FVector Loc0 = UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(), FVector(
                P0->GetNumberField(TEXT("x")), 0.0f, P0->GetNumberField(TEXT("z"))));
            FVector Loc1 = UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(), FVector(
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

                if (LineasISM)
                {
                    FTransform Transform;
                    Transform.SetLocation(LineaCentral.Posicion);
                    Transform.SetRotation(FQuat(FRotator(0.0f, Angle, 0.0f)));
                    Transform.SetScale3D(FVector(Largo / 100.0f, 0.1f, 0.02f));
                    LineasISM->AddInstance(Transform, true);
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

                if (CrucesISM)
                {
                    for (int32 s = 0; s < 5; s++)
                    {
                        FVector StripePos = Loc0 + Normal * (Cruce.Ancho * 0.5f - s * Cruce.Ancho / 5.0f);
                        StripePos.Z += 2.0f;
                        FTransform Transform;
                        Transform.SetLocation(StripePos);
                        Transform.SetRotation(FQuat(FRotator(0.0f, Angle, 0.0f)));
                        Transform.SetScale3D(FVector(3.0f, 0.3f, 0.02f));
                        CrucesISM->AddInstance(Transform, true);
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

                if (StopISM)
                {
                    FTransform Transform;
                    Transform.SetLocation(Stop.Posicion);
                    Transform.SetRotation(FQuat(FRotator(0.0f, Angle, 0.0f)));
                    Transform.SetScale3D(FVector(2.0f, RoadWidth * 0.8f, 0.02f));
                    StopISM->AddInstance(Transform, true);
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
