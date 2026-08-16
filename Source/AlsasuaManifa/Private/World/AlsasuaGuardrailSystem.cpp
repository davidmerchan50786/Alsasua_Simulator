#include "World/AlsasuaGuardrailSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "World/AlsasuaMallaFab.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"

void UAlsasuaGuardrailSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaGuardrailSystem::ColocarBarandillas()
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

    Barandillas.Empty();
    int32 Placed = 0;

    // Una capa instanciada, no un actor por tramo: son 761 tramos sólo en los
    // barrios de monte, y 1647 si se cuentan las calles residenciales.
    UStaticMesh* Malla = AlsasuaMallaFab::Resolver(TEXT("guarda_barandas"),
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (!Malla) return 0;

    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("Barandillas"));
#endif

    UHierarchicalInstancedStaticMeshComponent* Capa =
        NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, TEXT("ISM_Barandillas"));
    Capa->SetStaticMesh(Malla);
    Capa->SetupAttachment(Host->GetRootComponent());
    Capa->SetMobility(EComponentMobility::Static);
    // Colisión sí: una barandilla que no para es peor que ninguna.
    Capa->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Capa->SetCastShadow(false);
    Capa->RegisterComponent();

    for (const auto& RoadVal : *RoadsArr)
    {
        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        const FString Type = Road->HasField(TEXT("type")) ? Road->GetStringField(TEXT("type")) : TEXT("");
        const FString Barrio = Road->HasField(TEXT("barrio")) ? Road->GetStringField(TEXT("barrio")) : TEXT("");

        // La fase se llama "barandillas en puentes y zonas de riesgo", y eso es
        // Errota y Monte, que son los barrios de ladera. El tipo "bridge" no
        // existe en roads_unity.json —los puentes los construye UCargadorPuentes
        // desde waterways_unity.json— así que esa condición nunca casaba.
        //
        // "residential" sí casaba, y son 1023 de los 1647 tramos: quitamiedos de
        // carretera a lo largo de todas las calles del casco, que no es lo que
        // hay en Altsasu ni lo que la fase dice hacer. Queda detrás de una
        // bandera, apagada, en vez de borrado: si alguien lo quería, se enciende.
        const bool bZonaDeRiesgo = (Barrio == TEXT("Errota") || Barrio == TEXT("Monte"));
        const bool bNeedsGuardrail = bZonaDeRiesgo || (bEnCallesResidenciales && Type == TEXT("residential"));

        if (!bNeedsGuardrail) continue;

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

            float Offset = 300.0f;
            FVector GuardrailPos = Centro + Normal * Offset;
            GuardrailPos.Z += AlturaBarandilla;

            FGuardrail GR;
            GR.Inicio = Loc0;
            GR.Fin = Loc1;
            GR.Tipo = Type;
            GR.Barrio = Barrio;

            Capa->AddInstance(FTransform(FRotator(0.f, Angle, 0.f), GuardrailPos,
                FVector(Largo / 100.0f, 0.1f, AlturaBarandilla / 100.0f)), /*bWorldSpace=*/true);

            Barandillas.Add(GR);
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("Guardrails: %d barandillas en una capa instanciada (residenciales: %s)."),
        Placed, bEnCallesResidenciales ? TEXT("sí") : TEXT("no"));
    return Placed;
}
