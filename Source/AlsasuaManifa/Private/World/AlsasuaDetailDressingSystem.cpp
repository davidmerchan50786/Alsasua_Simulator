#include "World/AlsasuaDetailDressingSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaDetailDressingSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

void UAlsasuaDetailDressingSystem::CargarMueblesReales(UWorld* World)
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/street_furniture.json");
    FString JsonStr;
    if (!FFileHelper::LoadFileToString(JsonStr, *JsonPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("DetailDressing: No street_furniture.json, fallback procedural"));
        return;
    }

    TArray<TSharedPtr<FJsonValue>> Arr;
    TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Rd, Arr) || Arr.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("DetailDressing: JSON parse failed, fallback procedural"));
        return;
    }

    struct FMuebleDef
    {
        FString Tipo;
        FString Mesh;
        FVector Scale;
        FLinearColor Color;
    };

    const TMap<FString, FMuebleDef> Defs = {
        {TEXT("banco"),           {TEXT("banco"),        TEXT("/Game/EngineBasicShapes/Cube"),     FVector(1.2f, 0.5f, 0.8f),  FLinearColor(0.4f, 0.6f, 0.2f)}},
        {TEXT("bollard"),         {TEXT("bollard"),      TEXT("/Game/EngineBasicShapes/Cylinder"), FVector(0.15f, 0.15f, 0.6f), FLinearColor(0.3f, 0.3f, 0.3f)}},
        {TEXT("buzon_correos"),   {TEXT("buzon_correos"),TEXT("/Game/EngineBasicShapes/Cube"),     FVector(0.4f, 0.4f, 1.0f),  FLinearColor(0.8f, 0.1f, 0.1f)}},
        {TEXT("boca_incendio"),   {TEXT("boca_incendio"),TEXT("/Game/EngineBasicShapes/Cylinder"), FVector(0.2f, 0.2f, 0.6f),  FLinearColor(0.9f, 0.15f, 0.05f)}},
        {TEXT("parada_bus"),      {TEXT("parada_bus"),   TEXT("/Game/EngineBasicShapes/Cube"),     FVector(0.2f, 2.0f, 2.5f),  FLinearColor(0.1f, 0.3f, 0.8f)}},
        {TEXT("senal_stop"),     {TEXT("senal_stop"),   TEXT("/Game/EngineBasicShapes/Cube"),     FVector(0.6f, 0.05f, 0.8f), FLinearColor(0.9f, 0.2f, 0.0f)}},
        {TEXT("espejo_seguridad"),{TEXT("espejo_seguridad"),TEXT("/Game/EngineBasicShapes/Sphere"),FVector(0.3f, 0.3f, 0.3f),  FLinearColor(0.7f, 0.7f, 0.8f)}},
        {TEXT("bici_arbol"),      {TEXT("bici_arbol"),   TEXT("/Game/EngineBasicShapes/Cube"),     FVector(0.15f, 1.0f, 1.5f), FLinearColor(0.2f, 0.6f, 0.15f)}},
    };

    int32 Placed = 0;
    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const FString Tipo = Obj->GetStringField(TEXT("tipo"));
        const FMuebleDef* Def = Defs.Find(Tipo);
        if (!Def) continue;

        const float X = Obj->GetNumberField(TEXT("x"));
        const float Z = Obj->GetNumberField(TEXT("z"));
        const float Y = Obj->HasField(TEXT("y")) ? Obj->GetNumberField(TEXT("y")) : 0.0f;

        const FVector Pos = UAlsasuaGeoData::RelLocalToUE5(FVector(X, Y, Z));

        AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (!Actor) continue;

        Actor->SetMobility(EComponentMobility::Static);
        Actor->SetActorScale3D(Def->Scale);

        UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, *Def->Mesh);
        if (Mesh) Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);

        UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
        if (Mat) Actor->GetStaticMeshComponent()->SetMaterial(0, Mat);

#if WITH_EDITOR
        Actor->SetActorLabel(*FString::Printf(TEXT("Mueble_%s_%d"), *Tipo, Placed));
#endif

        FDetailItem Item;
        Item.Tipo = Tipo;
        Item.Posicion = Pos;
        Item.Rotacion = 0.0f;
        Item.Escala = 1.0f;
        Item.Color = Def->Color;
        Detalles.Add(Item);
        MueblesReales.Add(Actor);
        Placed++;
    }

    if (Placed > 0)
    {
        bUsandoDatosReales = true;
        UE_LOG(LogTemp, Log, TEXT("DetailDressing: %d muebles reales de street_furniture.json"), Placed);
    }
}

int32 UAlsasuaDetailDressingSystem::ColocarDetalle()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    CargarMueblesReales(World);

    if (!bUsandoDatosReales)
    {
        ColocarMacetas(World);
        ColocarBuzones(World);
        ColocarPapeleiras(World);
        ColocarBancos(World);
        ColocarVallasVerdes(World);
    }

    int32 Total = Detalles.Num();
    UE_LOG(LogTemp, Log, TEXT("DetailDressing: %d items de detalle colocados"), Total);
    return Total;
}

AStaticMeshActor* UAlsasuaDetailDressingSystem::CrearActor(
    UWorld* World, const FVector& Pos, float Rot, float Scale,
    const TCHAR* MeshPath, const TCHAR* MatPath, const FString& Label)
{
    AStaticMeshActor* Actor = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Pos, FRotator(0, Rot, 0));
    if (!Actor) return nullptr;

    Actor->SetMobility(EComponentMobility::Static);
    Actor->SetActorScale3D(FVector(Scale));

    UStaticMesh* Mesh = LoadObject<UStaticMesh>(nullptr, MeshPath);
    if (Mesh) Actor->GetStaticMeshComponent()->SetStaticMesh(Mesh);

    UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, MatPath);
    if (Mat) Actor->GetStaticMeshComponent()->SetMaterial(0, Mat);

#if WITH_EDITOR
    Actor->SetActorLabel(*Label);
#endif

    return Actor;
}

void UAlsasuaDetailDressingSystem::ColocarMacetas(UWorld* World)
{
    const TArray<FString> Barrios = {
        TEXT("Herriko"), TEXT("Zelai"), TEXT("SanPedro"), TEXT("Harrobieta")
    };

    for (int32 i = 0; i < MaxMacetas; i++)
    {
        FString Barrio = Barrios[FMath::RandRange(0, Barrios.Num() - 1)];
        FVector Centro = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(Barrio));

        FVector Pos = Centro + FVector(
            FMath::RandRange(-800.0f, 800.0f),
            FMath::RandRange(-800.0f, 800.0f), 0);

        float Escala = FMath::RandRange(0.8f, 1.5f);
        float Rot = FMath::RandRange(0.0f, 360.0f);

        FDetailItem Item;
        Item.Tipo = TEXT("maceta");
        Item.Posicion = Pos;
        Item.Rotacion = Rot;
        Item.Escala = Escala;
        Item.Barrio = Barrio;
        Item.Color = FLinearColor(0.6f, 0.3f, 0.15f);

        FString Label = FString::Printf(TEXT("Maceta_%s_%d"), *Barrio.Left(8), i);
        AStaticMeshActor* Actor = CrearActor(World, Pos, Rot, Escala,
            TEXT("/Game/EngineBasicShapes/Cylinder"),
            TEXT("/Game/Materiales/M_Piedra"), Label);

        if (Actor)
        {
            float S = Escala * 0.5f;
            Actor->SetActorScale3D(FVector(S, S, S * 1.5f));
        }

        Detalles.Add(Item);
    }
}

void UAlsasuaDetailDressingSystem::ColocarBuzones(UWorld* World)
{
    const TArray<FString> Calles = {
        TEXT("Kale Nagusia"), TEXT("Foruen Plaza"), TEXT("Iruñeko Etorbidea"),
        TEXT("San Pedro bidea"), TEXT("Geltokia kalea")
    };

    for (int32 i = 0; i < MaxBuzones; i++)
    {
        FString Calle = Calles[FMath::RandRange(0, Calles.Num() - 1)];
        FVector Pos = UAlsasuaGeoData::UnityaUnreal(FVector(
            1891.0f + FMath::RandRange(-5.0f, 5.0f),
            8570.0f + FMath::RandRange(-5.0f, 5.0f), 0));

        FDetailItem Item;
        Item.Tipo = TEXT("buzon");
        Item.Posicion = Pos;
        Item.Rotacion = FMath::RandRange(0.0f, 360.0f);
        Item.Escala = 0.7f;
        Item.Barrio = TEXT("Herriko");
        Item.Calle = Calle;
        Item.Color = FLinearColor(0.1f, 0.3f, 0.7f);

        FString Label = FString::Printf(TEXT("Buzon_%s_%d"), *Calle.Left(10), i);
        CrearActor(World, Pos, Item.Rotacion, 0.7f,
            TEXT("/Game/EngineBasicShapes/Cube"),
            TEXT("/Game/Materiales/M_Metal_Azul"), Label);

        Detalles.Add(Item);
    }
}

void UAlsasuaDetailDressingSystem::ColocarPapeleiras(UWorld* World)
{
    const TArray<FString> Barrios = {
        TEXT("Herriko"), TEXT("Zelai"), TEXT("Intxostia"), TEXT("SanPedro"),
        TEXT("Errota"), TEXT("Harrobieta"), TEXT("Ferroviario")
    };

    for (int32 i = 0; i < MaxPapeleiras; i++)
    {
        FString Barrio = Barrios[FMath::RandRange(0, Barrios.Num() - 1)];
        FVector Centro = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(Barrio));

        FVector Pos = Centro + FVector(
            FMath::RandRange(-1000.0f, 1000.0f),
            FMath::RandRange(-1000.0f, 1000.0f), 0);

        FDetailItem Item;
        Item.Tipo = TEXT("papelera");
        Item.Posicion = Pos;
        Item.Rotacion = FMath::RandRange(0.0f, 360.0f);
        Item.Escala = 0.6f;
        Item.Barrio = Barrio;
        Item.Color = FLinearColor(0.2f, 0.5f, 0.2f);

        FString Label = FString::Printf(TEXT("Papelera_%s_%d"), *Barrio.Left(8), i);
        CrearActor(World, Pos, Item.Rotacion, 0.6f,
            TEXT("/Game/EngineBasicShapes/Cube"),
            TEXT("/Game/Materiales/M_Verde_Oscuro"), Label);

        Detalles.Add(Item);
    }
}

void UAlsasuaDetailDressingSystem::ColocarBancos(UWorld* World)
{
    for (int32 i = 0; i < MaxBancos; i++)
    {
        FVector Pos = UAlsasuaGeoData::UnityaUnreal(FVector(
            1891.0f + FMath::RandRange(-6.0f, 6.0f),
            8571.0f + FMath::RandRange(-6.0f, 6.0f), 0));

        FDetailItem Item;
        Item.Tipo = TEXT("banco");
        Item.Posicion = Pos;
        Item.Rotacion = FMath::RandRange(0.0f, 360.0f);
        Item.Escala = 1.0f;
        Item.Barrio = TEXT("Herriko");
        Item.Color = FLinearColor(0.4f, 0.25f, 0.1f);

        FString Label = FString::Printf(TEXT("Banco_%d"), i);
        AStaticMeshActor* Actor = CrearActor(World, Pos, Item.Rotacion, 1.0f,
            TEXT("/Game/EngineBasicShapes/Cube"),
            TEXT("/Game/Materiales/M_Madera"), Label);

        if (Actor)
            Actor->SetActorScale3D(FVector(1.5f, 0.4f, 0.4f));

        Detalles.Add(Item);
    }
}

void UAlsasuaDetailDressingSystem::ColocarVallasVerdes(UWorld* World)
{
    for (int32 i = 0; i < MaxVallasVerdes; i++)
    {
        FString Barrio;
        if (i < 8) Barrio = TEXT("Zelai");
        else if (i < 14) Barrio = TEXT("Monte");
        else Barrio = TEXT("Intxostia");

        FVector Centro = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(Barrio));

        FVector Pos = Centro + FVector(
            FMath::RandRange(-1200.0f, 1200.0f),
            FMath::RandRange(-1200.0f, 1200.0f), 0);

        FDetailItem Item;
        Item.Tipo = TEXT("valla_verde");
        Item.Posicion = Pos;
        Item.Rotacion = FMath::RandRange(0.0f, 360.0f);
        Item.Escala = 1.2f;
        Item.Barrio = Barrio;
        Item.Color = FLinearColor(0.15f, 0.55f, 0.15f);

        FString Label = FString::Printf(TEXT("VallaVerde_%s_%d"), *Barrio.Left(8), i);
        AStaticMeshActor* Actor = CrearActor(World, Pos, Item.Rotacion, 1.2f,
            TEXT("/Game/EngineBasicShapes/Cube"),
            TEXT("/Game/Materiales/M_Seto"), Label);

        if (Actor)
            Actor->SetActorScale3D(FVector(3.0f, 0.15f, 1.0f));

        Detalles.Add(Item);
    }
}
