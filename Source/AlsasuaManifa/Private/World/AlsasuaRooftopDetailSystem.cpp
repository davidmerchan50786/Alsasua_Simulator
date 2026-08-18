#include "World/AlsasuaRooftopDetailSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
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
#include "Materials/MaterialInterface.h"
#include "Math/RandomStream.h"
#include "AlturasLidarComun.h"

void UAlsasuaRooftopDetailSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaRooftopDetailSystem::ColocarDetallesCubierta()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/buildings_final.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath)) return 0;

    TArray<TSharedPtr<FJsonValue>> BuildingsArr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, BuildingsArr) || BuildingsArr.Num() == 0) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    Items.Empty();
    int32 Placed = 0;

    // Una capa instanciada por tipo de remate. Antes era un AStaticMeshActor por
    // pieza —del orden de mil— y, dentro de la lambda que lo creaba, un
    // LoadObject de malla y otro de material POR PIEZA.
    if (Host) Host->Destroy();
    Host = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
    if (!Host) return 0;
    Host->SetRootComponent(NewObject<USceneComponent>(Host, TEXT("Raiz")));
    Host->GetRootComponent()->RegisterComponent();
#if WITH_EDITOR
    Host->SetActorLabel(TEXT("Cubiertas"));
#endif

    TMap<FString, UHierarchicalInstancedStaticMeshComponent*> Capas;
    auto CapaDe = [&](const FString& Tipo, const TCHAR* MeshPath, const TCHAR* MatPath)
        -> UHierarchicalInstancedStaticMeshComponent*
    {
        if (UHierarchicalInstancedStaticMeshComponent** F = Capas.Find(Tipo)) return *F;

        // Si hay algo bajado de Fab para ese tipo se prefiere; si no, la
        // primitiva del motor que ya se usaba.
        UStaticMesh* M = AlsasuaMallaFab::Resolver(Tipo, MeshPath);
        if (!M) return nullptr;

        UHierarchicalInstancedStaticMeshComponent* C =
            NewObject<UHierarchicalInstancedStaticMeshComponent>(Host, *(TEXT("ISM_") + Tipo));
        C->SetStaticMesh(M);
        if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, MatPath))
            C->SetMaterial(0, Mat);
        C->SetupAttachment(Host->GetRootComponent());
        C->SetMobility(EComponentMobility::Static);
        // Trastos de cubierta: no se pisan y su sombra no se ve desde la calle.
        C->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        C->SetCastShadow(false);
        C->RegisterComponent();
        Capas.Add(Tipo, C);
        return C;
    };

    for (const auto& BldVal : BuildingsArr)
    {
        const TSharedPtr<FJsonObject>& Bld = BldVal->AsObject();
        if (!Bld) continue;

        const int32 Id = Bld->GetIntegerField(TEXT("id"));
        float Height = Bld->HasField(TEXT("height")) ? Bld->GetNumberField(TEXT("height")) : 10.0f;
        const FString Barrio = Bld->HasField(TEXT("barrio")) ? Bld->GetStringField(TEXT("barrio")) : TEXT("");
        const FString RoofTipo = Bld->HasField(TEXT("roof_tipo_real")) ? Bld->GetStringField(TEXT("roof_tipo_real")) : TEXT("desconocido");

        const TArray<TSharedPtr<FJsonValue>>* VertsArr;
        if (!Bld->TryGetArrayField(TEXT("vertices"), VertsArr) || !VertsArr || VertsArr->Num() < 3) continue;

        float CX = 0, CZ = 0;
        for (const auto& V : *VertsArr)
        {
            const TSharedPtr<FJsonObject>& Vert = V->AsObject();
            if (!Vert) continue;
            CX += Vert->GetNumberField(TEXT("x"));
            CZ += Vert->GetNumberField(TEXT("z"));
        }
        CX /= VertsArr->Num();
        CZ /= VertsArr->Num();

        // Dos correcciones que se suman:
        //  - La cota parte del SUELO (RelLocalASueloUE5) y no de Z=0, o las
        //    antenas y depósitos salían a media montaña por debajo del pueblo.
        //  - La altura es la medida por LiDAR y no la de OSM, ~3 m más baja, con
        //    la que las chimeneas quedaban enterradas dentro del tejado.
        {
            float AltLidar = 0.f;
            int32 PlantasLidar = 0;
            const FVector Plano = UAlsasuaGeoData::RelLocalToUE5(FVector(CX, 0.0f, CZ));
            if (AlturasLidar::Buscar(FVector2D(Plano.X, Plano.Y), AltLidar, PlantasLidar))
                Height = AltLidar;
        }

        FVector RoofCenter = UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(),
            FVector(CX, 0.0f, CZ), Height * 100.0f);

        const bool bFlatRoof = RoofTipo.Contains(TEXT("cemento"));
        const bool bPitchedRoof = RoofTipo.Contains(TEXT("pizarra")) || RoofTipo.Contains(TEXT("teja"));

        // Sorteo por id, no FRand: si no, la cubierta cambia en cada arranque y
        // el pueblo no es el mismo dos veces.
        FRandomStream Sorteo(Id * 2654435761u + 7);

        auto CrearItem = [&](const FString& Tipo, const TCHAR* MeshPath,
            const TCHAR* MatPath, float SX, float SY, float SZ, float OffX, float OffZ)
        {
            UHierarchicalInstancedStaticMeshComponent* Capa = CapaDe(Tipo, MeshPath, MatPath);
            if (!Capa) return;

            const FVector Pos = RoofCenter + FVector(OffX, OffZ, 0);
            const float Yaw = Sorteo.FRandRange(0.f, 360.f);
            Capa->AddInstance(FTransform(FRotator(0.f, Yaw, 0.f), Pos, FVector(SX, SY, SZ)),
                /*bWorldSpace=*/true);

            FRooftopItem Item;
            Item.BuildingId = Id;
            Item.Tipo = Tipo;
            Item.Posicion = Pos;
            Item.Rotacion = Yaw;
            Item.Escala = SX;
            Item.Barrio = Barrio;
            Items.Add(Item);
            Placed++;
        };

        if (bPitchedRoof && Sorteo.GetFraction() < 0.4f)
        {
            float OffX = Sorteo.FRandRange(-200.0f, 200.0f);
            float OffZ = Sorteo.FRandRange(-200.0f, 200.0f);
            CrearItem(TEXT("antena"),
                TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                0.05f, 0.05f, 3.0f, OffX, OffZ);
        }

        // Las chimeneas las pone UAlsasuaTejadoModular con la pieza del kit
        // (Roof_Prop_Chimney_Stone) apoyada en la cumbrera real del edificio.
        // Aquí eran un cubo del motor en un punto al azar de la cubierta.

        if (bFlatRoof && Sorteo.GetFraction() < 0.3f)
        {
            float OffX = Sorteo.FRandRange(-100.0f, 100.0f);
            float OffZ = Sorteo.FRandRange(-100.0f, 100.0f);
            CrearItem(TEXT("deposito_agua"),
                TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                1.0f, 1.0f, 1.5f, OffX, OffZ);
        }

        if (bFlatRoof && Sorteo.GetFraction() < 0.2f)
        {
            float OffX = Sorteo.FRandRange(-200.0f, 200.0f);
            float OffZ = Sorteo.FRandRange(-200.0f, 200.0f);
            CrearItem(TEXT("placa_solar"),
                TEXT("/Engine/BasicShapes/Plane.Plane"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                2.0f, 1.5f, 0.02f, OffX, OffZ);
        }

        if (Sorteo.GetFraction() < 0.15f)
        {
            float OffX = Sorteo.FRandRange(-150.0f, 150.0f);
            float OffZ = Sorteo.FRandRange(-150.0f, 150.0f);
            CrearItem(TEXT("satelital"),
                TEXT("/Engine/BasicShapes/Cylinder.Cylinder"),
                TEXT("/Engine/EngineMaterials/DefaultMaterial"),
                0.3f, 0.3f, 0.8f, OffX, OffZ);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Rooftop: %d detalles de cubierta (antenas, chimeneas, depósitos, placas solares)"), Placed);
    return Placed;
}
