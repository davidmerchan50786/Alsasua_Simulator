#include "World/AlsasuaRoadSurfaceSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaRoadSurfaceSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarSuperficies();
}

void UAlsasuaRoadSurfaceSystem::Deinitialize()
{
    Superficies.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaRoadSurfaceSystem::CargarSuperficies()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RoadSurface: No se pudo cargar roads_unity.json"));
        return false;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

    const TArray<TSharedPtr<FJsonValue>>* Arr;
    if (!Root->TryGetArrayField(TEXT("roads"), Arr) && !Root->TryGetArrayField(TEXT(""), Arr)) return false;

    Superficies.Empty(Arr->Num());
    for (const auto& Val : *Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Obj->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() == 0) continue;
        const TSharedPtr<FJsonObject>& FirstPt = (*PointsArr)[0]->AsObject();
        if (!FirstPt) continue;

        FRoadSurfaceEntry Entry;
        Entry.Nombre = Obj->HasField(TEXT("name")) ? Obj->GetStringField(TEXT("name")) : TEXT("");
        Entry.Calle = Entry.Nombre;
        Entry.X = FirstPt->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX;
        Entry.Z = FirstPt->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ;
        Entry.Ancho = Obj->HasField(TEXT("width")) ? Obj->GetNumberField(TEXT("width")) : 8.0f;
        Entry.Barrio = Obj->HasField(TEXT("barrio")) ? Obj->GetStringField(TEXT("barrio")) : TEXT("");
        Entry.Tipo = Obj->HasField(TEXT("type")) ? Obj->GetStringField(TEXT("type")) : TEXT("residential");

        Entry.Material = TEXT("asphalt");
        if (Entry.Tipo == TEXT("pedestrian") || Entry.Tipo == TEXT("footway"))
            Entry.Material = TEXT("cobblestone");
        else if (Entry.Tipo == TEXT("service"))
            Entry.Material = TEXT("asphalt_worn");
        else if (Entry.Tipo == TEXT("unclassified") || Entry.Tipo == TEXT("track"))
            Entry.Material = TEXT("gravel");
        else if (Entry.Barrio == TEXT("Herriko") || Entry.Barrio == TEXT("Harrobieta"))
            Entry.Material = TEXT("cobblestone");
        else if (Entry.Ancho < 5.0f)
            Entry.Material = TEXT("asphalt_worn");

        Superficies.Add(Entry);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("RoadSurface: %d tramos con superficie asignada"), Superficies.Num());
    return true;
}

int32 UAlsasuaRoadSurfaceSystem::AplicarSuperficiesEnMundo()
{
    if (!bCargado && !CargarSuperficies()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    static TMap<FString, UStaticMesh*> MaterialMeshes;
    if (MaterialMeshes.Num() == 0)
    {
        MaterialMeshes.Add(TEXT("asphalt"), LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Cube.Cube")));
        MaterialMeshes.Add(TEXT("cobblestone"), LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Cube.Cube")));
        MaterialMeshes.Add(TEXT("asphalt_worn"), LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Cube.Cube")));
        MaterialMeshes.Add(TEXT("gravel"), LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Cube.Cube")));
    }

    static TMap<FString, FLinearColor> MaterialColors;
    if (MaterialColors.Num() == 0)
    {
        MaterialColors.Add(TEXT("asphalt"), FLinearColor(0.15f, 0.15f, 0.15f));
        MaterialColors.Add(TEXT("cobblestone"), FLinearColor(0.45f, 0.40f, 0.35f));
        MaterialColors.Add(TEXT("asphalt_worn"), FLinearColor(0.25f, 0.24f, 0.23f));
        MaterialColors.Add(TEXT("gravel"), FLinearColor(0.55f, 0.50f, 0.45f));
    }

    int32 Placed = 0;

    // Fase 5: leer wetness del MPC global para asfalto mojado.
    float GlobalWetness = 0.f;
    if (UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr,
        TEXT("/Game/Materiales/MPC_Clima.MPC_Clima")))
    {
        if (UMaterialParameterCollectionInstance* Inst = World->GetParameterCollectionInstance(MPC))
        {
            Inst->GetScalarParameterValue(FName("GlobalWetness"), GlobalWetness);
        }
    }

    for (const FRoadSurfaceEntry& Entry : Superficies)
    {
        FVector Loc = UAlsasuaGeoData::UnityaUnreal(FVector(Entry.X, Entry.Z, 0));

        AStaticMeshActor* RoadActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator::ZeroRotator);
        if (RoadActor)
        {
            RoadActor->SetMobility(EComponentMobility::Movable);

            float ScaleX = Entry.Ancho * 100.0f;
            RoadActor->SetActorScale3D(FVector(ScaleX / 100.0f, ScaleX / 100.0f, 0.1f));

            UStaticMesh** Mesh = MaterialMeshes.Find(Entry.Material);
            if (Mesh && *Mesh)
                RoadActor->GetStaticMeshComponent()->SetStaticMesh(*Mesh);

            FLinearColor* Color = MaterialColors.Find(Entry.Material);
            if (Color)
            {
                // Fase 3: desgaste por zona ÔÇö intersecciones y v├¡as anchas m├ís oscuras.
                float WearFactor = 0.f;
                if (Entry.Tipo == TEXT("primary") || Entry.Tipo == TEXT("trunk"))
                    WearFactor = 0.25f;  // avenidas principales
                else if (Entry.Tipo == TEXT("tertiary") || Entry.Tipo == TEXT("residential"))
                    WearFactor = 0.10f;  // residencial
                else if (Entry.Ancho > 10.0f)
                    WearFactor = 0.15f;  // calles anchas

                const FLinearColor WornColor = FMath::Lerp(*Color, FLinearColor(0.18f, 0.17f, 0.16f), WearFactor);

                if (UMaterialInstanceDynamic* DynMat = RoadActor->GetStaticMeshComponent()->CreateDynamicMaterialInstance(0))
                {
                    DynMat->SetVectorParameterValue(FName(TEXT("Color")), WornColor);
                    // Fase 5: asfalto mojado ÔÇö Roughness baja cuando llueve.
                    DynMat->SetScalarParameterValue(FName(TEXT("Wetness")), GlobalWetness);
                }
            }

#if WITH_EDITOR
            RoadActor->SetActorLabel(*FString::Printf(TEXT("Road_%s_%s"),
                *Entry.Material, *Entry.Nombre.Left(20)));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("RoadSurface: %d tramos de calle colocados"), Placed);
    return Placed;
}
