#include "World/AlsasuaSidewalkSystem.h"
#include "AlsasuaMallaFab.h"
#include "World/AlsasuaTerrainLayersSystem.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "Components/SceneComponent.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"

void UAlsasuaSidewalkSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaSidewalkSystem::GenerarAceras()
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

    int32 Placed = 0;
    Acera.Empty();

    // Tabla de firme por barrio. Si el subsistema no está, se sigue con la regla
    // de siempre en vez de dejar el pueblo entero de hormigón.
    const UAlsasuaTerrainLayersSystem* Firme = World->GetGameInstance()
        ? World->GetGameInstance()->GetSubsystem<UAlsasuaTerrainLayersSystem>() : nullptr;

    // --- Una capa instanciada por acabado, no un actor por losa ------------
    // Los materiales se resuelven UNA vez. Antes se hacían dos LoadObject por
    // losa, dentro del bucle: doce mil cargas para seis mil losas.
    auto CargarAcabado = [](const TCHAR* Preferido) -> UMaterialInterface*
    {
        if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, Preferido)) return M;
        if (UMaterialInterface* M = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Pavimento"))) return M;
        return LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Terreno_Acera.M_Terreno_Acera"));
    };
    UMaterialInterface* MatPiedra   = CargarAcabado(TEXT("/Game/Materiales/M_Acera_Piedra"));
    UMaterialInterface* MatHormigon = CargarAcabado(TEXT("/Game/Materiales/M_Acera_Hormigon"));

    UStaticMesh* Losa = AlsasuaMallaFab::Resolver(TEXT("acera_pieza"),
        TEXT("/Engine/BasicShapes/Plane.Plane"));
    if (!Losa)
    {
        UE_LOG(LogTemp, Warning, TEXT("Sidewalks: sin malla de acera; no se generan."));
        return 0;
    }

    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("Aceras"));
#endif

    auto CrearCapa = [&](const TCHAR* Nombre, UMaterialInterface* Mat)
    {
        UHierarchicalInstancedStaticMeshComponent* C =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, Nombre);
        C->SetStaticMesh(Losa);
        if (Mat) C->SetMaterial(0, Mat);
        C->SetupAttachment(Host->GetRootComponent());
        C->SetMobility(EComponentMobility::Static);
        // La acera es suelo: se pisa, así que colisión sí. Sombra dinámica no,
        // que a este número de instancias es lo que se come el frame y una losa
        // plana pegada al suelo no proyecta nada que se vea.
        C->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
        C->SetCastShadow(false);
        C->RegisterComponent();
        return C;
    };
    UHierarchicalInstancedStaticMeshComponent* CapaPiedra   = CrearCapa(TEXT("ISM_Acera_Piedra"), MatPiedra);
    UHierarchicalInstancedStaticMeshComponent* CapaHormigon = CrearCapa(TEXT("ISM_Acera_Hormigon"), MatHormigon);

    for (const auto& RoadVal : *RoadsArr)
    {
        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        const FString Type = Road->HasField(TEXT("type")) ? Road->GetStringField(TEXT("type")) : TEXT("");
        if (Type == TEXT("footway") || Type == TEXT("path") || Type == TEXT("track")) continue;

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

            float OffsetX = RoadWidth * 100.0f * 0.5f + AnchoAceras * 0.5f;

            for (int32 Side = -1; Side <= 1; Side += 2)
            {
                FVector SidewalkCenter = Centro + Normal * OffsetX * Side;
                SidewalkCenter.Z += AlturaBordillo;

                const float Angle = FMath::RadiansToDegrees(FMath::Atan2(Direccion.Y, Direccion.X));

                // Una instancia, no un actor. Misma transformada que tenía el
                // actor: el plano del motor mide 100 uu, así que escalar por
                // Largo/100 y Ancho/100 le da el tamaño del tramo.
                const FTransform T(FRotator(0.f, Angle, 0.f), SidewalkCenter,
                    FVector(Largo / 100.0f, AnchoAceras / 100.0f, 0.15f));

                // Qué barrio va empedrado lo dice AlsasuaTerrainLayersSystem,
                // que es quien tiene la tabla de firme por barrio. Estaba
                // copiado aquí como "Herriko o Harrobieta", y una regla de
                // pueblo en dos sitios es una regla que se separa.
                const bool bPiedra = Firme ? Firme->BarrioEmpedrado(Barrio)
                                           : (Barrio == TEXT("Herriko") || Barrio == TEXT("Harrobieta"));
                (bPiedra ? CapaPiedra : CapaHormigon)->AddInstance(T, /*bWorldSpace=*/true);

                FSidewalkSegment Seg;
                Seg.Inicio = Loc0;
                Seg.Fin = Loc1;
                Seg.Ancho = AnchoAceras;
                Seg.Calle = Calle;
                Seg.Barrio = Barrio;
                Seg.Altura = AlturaBordillo;
                Acera.Add(Seg);

                Placed++;
            }
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("Sidewalks: %d losas de acera en 2 capas instanciadas (%d piedra, %d hormigón)."),
        Placed, CapaPiedra->GetInstanceCount(), CapaHormigon->GetInstanceCount());
    return Placed;
}
