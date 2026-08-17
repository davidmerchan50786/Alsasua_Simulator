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

    // Cuántas vías podrían llevar paso de cebra y línea de stop. Sólo mira el
    // tipo, sin geometría, y sirve para repartir el tope por todo el pueblo.
    //
    // Sin esto, el tope se gasta en las primeras vías del fichero —que no viene
    // ordenado por nada geográfico— y el resto se queda sin una sola marca: 30
    // pasos de cebra de 269 candidatas y 20 líneas de stop de 194, todos
    // amontonados al principio del recorrido.
    int32 CandCruces = 0, CandStop = 0;
    for (const auto& RV : *RoadsArr)
    {
        const TSharedPtr<FJsonObject>& R = RV->AsObject();
        if (!R) continue;
        const FString T = R->HasField(TEXT("type")) ? R->GetStringField(TEXT("type")) : TEXT("");
        if (T == TEXT("residential") || T == TEXT("tertiary")) ++CandCruces;
        if (T == TEXT("residential")) ++CandStop;
    }
    int32 VistasCruce = 0, VistasStop = 0;

    // Se admite la marca sólo mientras vaya por detrás de su cuota, que es lo
    // que reparte los huecos por todo el trazado en vez de cortar de golpe.
    auto TocaAhora = [](int32 Vistas, int32 Candidatas, int32 Puestas, int32 Tope) -> bool
    {
        if (Candidatas <= 0) return false;
        const int32 Objetivo = FMath::Min(Tope, Candidatas);
        return Puestas <= static_cast<int32>(static_cast<int64>(Vistas) * Objetivo / Candidatas);
    };

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

            // Línea central: en las de doble sentido con calzada de verdad, o
            // sea las 75 tertiary —las salidas a Gipuzkoa y Urdiain— y la A-10.
            //
            // El criterio era `RoadWidth >= 6`, y mirando los anchos que hay en
            // roads_unity.json eso son exactamente la autovía (11 m) y sus 50
            // enlaces (6 m): la línea central se pintaba SÓLO en la autovía y en
            // ninguna calle. Y un enlace es de sentido único, así que ahí
            // tampoco va.
            const bool bDobleSentido = (Type == TEXT("tertiary") || Type == TEXT("motorway"));

            if (bDobleSentido && TotalLineas < MaxLineasCentrales && Largo > 300.0f)
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

            // Paso de cebra en el arranque del trazado. OSM parte las vías en
            // los cruces, así que el primer punto de un tramo suele ser un nudo,
            // que es donde va un paso.
            //
            // Faltaba el filtro por tipo: se pintaba en el arranque de
            // cualquiera de las 489 vías, la A-10 y sus 50 enlaces incluidos.
            // Un paso de peatones en la autovía.
            const bool bCalleDePueblo = (Type == TEXT("residential") || Type == TEXT("tertiary"));

            bool bTocaCruce = false;
            if (bCalleDePueblo && i == 0)
            {
                bTocaCruce = TocaAhora(VistasCruce, CandCruces, TotalCruces, MaxCrucesPeatonales);
                ++VistasCruce;
            }

            if (bTocaCruce)
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
                    // Las cinco bandas, centradas en la calzada. El reparto era
                    // `W/2 - s*W/5` para s de 0 a 4, o sea de +0,5W a -0,3W: el
                    // paso quedaba descentrado un décimo del ancho, con una
                    // banda fuera de la calzada por un lado y hueco por el otro.
                    const int32 NumBandas = 5;
                    for (int32 s = 0; s < NumBandas; s++)
                    {
                        const float Desvio = Cruce.Ancho * ((s + 0.5f) / NumBandas - 0.5f);
                        FVector StripePos = Loc0 + Normal * Desvio;
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

            // Línea de stop en el final del trazado: OSM parte las vías en los
            // cruces, así que el último punto de un tramo residencial es la
            // salida a otra calle.
            bool bTocaStop = false;
            if (Type == TEXT("residential") && i == PointsArr->Num() - 2)
            {
                bTocaStop = TocaAhora(VistasStop, CandStop, TotalStop, MaxLineasStop);
                ++VistasStop;
            }

            if (bTocaStop)
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
