#include "World/AlsasuaFarolaPlacer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaFarolaPlacer::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarFarolas();
}

void UAlsasuaFarolaPlacer::Deinitialize()
{
    Farolas.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaFarolaPlacer::CargarFarolas()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/street_furniture.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("FarolaPlacer: No se pudo cargar street_furniture.json"));
        return false;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

    const TArray<TSharedPtr<FJsonValue>>* ItemsArr;
    if (!Root->TryGetArrayField(TEXT("items"), ItemsArr)) return false;

    Farolas.Empty(ItemsArr->Num());
    for (const auto& Val : *ItemsArr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const FString Type = Obj->GetStringField(TEXT("type"));
        if (Type != TEXT("farola") && Type != TEXT("farola_decorativa")) continue;

        FFarolaEntry F;
        F.Calle = Obj->HasField(TEXT("calle")) ? Obj->GetStringField(TEXT("calle")) : TEXT("");
        F.X = Obj->GetNumberField(TEXT("x"));
        F.Z = Obj->GetNumberField(TEXT("z"));
        F.Rotacion = Obj->HasField(TEXT("rotacion")) ? Obj->GetNumberField(TEXT("rotacion")) : 0.0f;
        F.TipoFarola = Type.Contains(TEXT("decorativa")) ? TEXT("forjado_tradicional") : TEXT("tradicional");
        F.AlturaM = Obj->HasField(TEXT("altura_m")) ? Obj->GetNumberField(TEXT("altura_m")) : 3.5f;

        Farolas.Add(F);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("FarolaPlacer: %d farolas reales cargadas"), Farolas.Num());
    return true;
}

int32 UAlsasuaFarolaPlacer::ColocarFarolasEnMundo()
{
    if (!bCargado && !CargarFarolas()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Placed = 0;

    UStaticMesh* LampMesh1 = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/AssetsImportados/Farolas/StreetLamps2/StreetLampRound1"));
    UStaticMesh* LampMesh2 = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Game/AssetsImportados/Farolas/StreetLamps2/StreetLampRound2"));

    for (const FFarolaEntry& F : Farolas)
    {
        FVector Loc = UAlsasuaGeoData::UnityaUnreal(FVector(F.X + UAlsasuaGeoData::OX, 0.0f, F.Z + UAlsasuaGeoData::OZ));
        Loc.Z += F.AlturaM * 50.0f;

        AStaticMeshActor* FarolaActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0, F.Rotacion, 0));
        if (FarolaActor)
        {
            FarolaActor->SetMobility(EComponentMobility::Movable);

            UStaticMesh* MeshToUse = (Placed % 2 == 0) ? LampMesh1 : LampMesh2;
            if (MeshToUse)
                FarolaActor->GetStaticMeshComponent()->SetStaticMesh(MeshToUse);

#if WITH_EDITOR
            FarolaActor->SetActorLabel(*FString::Printf(TEXT("Farola_%s_%d"),
                *F.Calle, Placed));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("FarolaPlacer: %d farolas reales colocadas"), Placed);
    return Placed;
}
