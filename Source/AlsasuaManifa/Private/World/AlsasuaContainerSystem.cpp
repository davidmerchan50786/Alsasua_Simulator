#include "World/AlsasuaContainerSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaContainerSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaContainerSystem::ColocarContenedores()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Contenedores.Empty();

    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/street_furniture.json");
    TArray<FString> Lines;
    if (!FFileHelper::LoadFileToStringArray(Lines, *JsonPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("Containers: No se pudo cargar street_furniture.json, usando fallback"));
        return ColocarContenedoresFallback();
    }

    FString Js;
    for (const FString& L : Lines) Js += L;

    TArray<TSharedPtr<FJsonValue>> Arr;
    TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Js);
    if (!FJsonSerializer::Deserialize(Rd, Arr)) return ColocarContenedoresFallback();

    const TArray<TPair<FString, FLinearColor>> TiposContenedor = {
        {TEXT("resto"), FLinearColor(0.3f, 0.3f, 0.3f)},
        {TEXT("papel"), FLinearColor(0.0f, 0.3f, 0.7f)},
        {TEXT("plastico"), FLinearColor(0.0f, 0.5f, 0.0f)},
        {TEXT("vidrio"), FLinearColor(0.1f, 0.6f, 0.1f)},
        {TEXT("organico"), FLinearColor(0.5f, 0.3f, 0.1f)},
    };

    int32 Placed = 0;
    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const FString Type = Obj->GetStringField(TEXT("type"));
        if (Type != TEXT("papelera") && Type != TEXT("papelera_reciclaje")) continue;

        const float X = Obj->GetNumberField(TEXT("x"));
        const float Z = Obj->GetNumberField(TEXT("z"));
        const FString Barrio = Obj->GetStringField(TEXT("barrio"));
        const float Rot = Obj->HasField(TEXT("rotacion")) ? Obj->GetNumberField(TEXT("rotacion")) : 0.0f;

        const FVector Pos = UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(), FVector(X, 0.0f, Z));

        int32 TipoIdx = Placed % TiposContenedor.Num();
        const auto& TipoInfo = TiposContenedor[TipoIdx];

        FContainer Cont;
        Cont.Tipo = TipoInfo.Key;
        Cont.Posicion = Pos;
        Cont.Rotacion = Rot;
        Cont.Barrio = Barrio;

        AStaticMeshActor* ContActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator(0, Rot, 0));
        if (ContActor)
        {
            ContActor->SetMobility(EComponentMobility::Static);
            ContActor->SetActorScale3D(FVector(1.2f, 0.8f, 1.0f));

            UStaticMesh* TrashcanMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/CitySample/Prop/Kit_Trashcan_A/Mesh/SM_Trashcan_A_01"));
            if (TrashcanMesh)
                ContActor->GetStaticMeshComponent()->SetStaticMesh(TrashcanMesh);

            UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr,
                TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
            if (Mat) ContActor->GetStaticMeshComponent()->SetMaterial(0, Mat);

#if WITH_EDITOR
            ContActor->SetActorLabel(*FString::Printf(TEXT("Contenedor_%s_%s_%d"),
                *TipoInfo.Key, *Barrio.Left(6), Placed));
#endif
        }

        Contenedores.Add(Cont);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Containers: %d contenedores reales de street_furniture.json"), Placed);
    return Placed;
}

int32 UAlsasuaContainerSystem::ColocarContenedoresFallback()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    Contenedores.Empty();

    const TArray<FString> Barrios = {
        TEXT("Herriko"), TEXT("Zelai"), TEXT("Intxostia"), TEXT("SanPedro"),
        TEXT("Errota"), TEXT("Harrobieta"), TEXT("Ferroviario"), TEXT("Monte")
    };

    const TArray<TPair<FString, FLinearColor>> TiposContenedor = {
        {TEXT("resto"), FLinearColor(0.3f, 0.3f, 0.3f)},
        {TEXT("papel"), FLinearColor(0.0f, 0.3f, 0.7f)},
        {TEXT("plastico"), FLinearColor(0.0f, 0.5f, 0.0f)},
        {TEXT("vidrio"), FLinearColor(0.1f, 0.6f, 0.1f)},
        {TEXT("organico"), FLinearColor(0.5f, 0.3f, 0.1f)},
    };

    int32 Placed = 0;
    for (int32 i = 0; i < MaxContenedores; i++)
    {
        FString Barrio = Barrios[FMath::RandRange(0, Barrios.Num() - 1)];
        FVector Centro = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(Barrio));
        FVector Pos = Centro + FVector(FMath::RandRange(-1000.0f, 1000.0f), FMath::RandRange(-1000.0f, 1000.0f), 0);
        Pos.Z = UAlsasuaGeoData::AlturaSueloUE5(GetWorld(), Pos.X, Pos.Y);

        const auto& TipoInfo = TiposContenedor[FMath::RandRange(0, TiposContenedor.Num() - 1)];
        float Rot = FMath::RandRange(0.0f, 360.0f);

        FContainer Cont;
        Cont.Tipo = TipoInfo.Key;
        Cont.Posicion = Pos;
        Cont.Rotacion = Rot;
        Cont.Barrio = Barrio;

        AStaticMeshActor* ContActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator(0, Rot, 0));
        if (ContActor)
        {
            ContActor->SetMobility(EComponentMobility::Static);
            ContActor->SetActorScale3D(FVector(1.2f, 0.8f, 1.0f));
            // Ruta constante: se resuelve una vez y no por contenedor.
            static UStaticMesh* const TrashcanMesh = LoadObject<UStaticMesh>(nullptr,
                TEXT("/Game/CitySample/Prop/Kit_Trashcan_A/Mesh/SM_Trashcan_A_01"));
            if (TrashcanMesh) ContActor->GetStaticMeshComponent()->SetStaticMesh(TrashcanMesh);
#if WITH_EDITOR
            ContActor->SetActorLabel(*FString::Printf(TEXT("Contenedor_%s_%s_%d"), *TipoInfo.Key, *Barrio.Left(6), i));
#endif
        }
        Contenedores.Add(Cont);
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Containers: %d contenedores fallback aleatorios"), Placed);
    return Placed;
}
