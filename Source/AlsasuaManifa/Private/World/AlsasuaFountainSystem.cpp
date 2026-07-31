#include "World/AlsasuaFountainSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/AudioComponent.h"
#include "GeoDataAlsasua.h"
#include "Kismet/KismetMathLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UAlsasuaFountainSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
}

int32 UAlsasuaFountainSystem::ColocarFuentes()
{
    Fuentes.Empty();

    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/street_furniture.json");
    TArray<FString> Lines;
    bool bLoaded = false;

    if (FFileHelper::LoadFileToStringArray(Lines, *JsonPath))
    {
        FString Js;
        for (const FString& L : Lines) Js += L;

        TArray<TSharedPtr<FJsonValue>> Arr;
        TSharedRef<TJsonReader<>> Rd = TJsonReaderFactory<>::Create(Js);
        if (FJsonSerializer::Deserialize(Rd, Arr))
        {
            for (const auto& Val : Arr)
            {
                const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
                if (!Obj) continue;
                if (Obj->GetStringField(TEXT("type")) != TEXT("fuente")) continue;

                FRealFountain Fuente;
                Fuente.Nombre = Obj->GetStringField(TEXT("nombre"));
                Fuente.Calle = Obj->HasField(TEXT("calle")) ? Obj->GetStringField(TEXT("calle")) : TEXT("");
                Fuente.Barrio = Obj->HasField(TEXT("barrio")) ? Obj->GetStringField(TEXT("barrio")) : TEXT("Herriko");

                const float X = Obj->GetNumberField(TEXT("x"));
                const float Z = Obj->GetNumberField(TEXT("z"));
                Fuente.Posicion = UAlsasuaGeoData::AbsLocalToUE5(FVector(X, 0.0f, Z));
                Fuente.Radio = 100.0f;
                Fuente.AlturaCazoleta = 80.0f;
                Fuente.bFuncional = Obj->HasField(TEXT("activa")) ? Obj->GetBoolField(TEXT("activa")) : true;
                Fuentes.Add(Fuente);
                bLoaded = true;
            }
        }
    }

    if (!bLoaded)
    {
        const TArray<TPair<FString, FString>> CallesFuentes = {
            {TEXT("Foruen Plaza"), TEXT("Herriko")},
            {TEXT("Kale Nagusia"), TEXT("Herriko")},
            {TEXT("Iruñeko Etorbidea"), TEXT("Zelai")},
            {TEXT("San Pedro bidea"), TEXT("SanPedro")},
            {TEXT("Harrobieta kalea"), TEXT("Harrobieta")},
        };

        for (int32 i = 0; i < CallesFuentes.Num(); i++)
        {
            FRealFountain Fuente;
            Fuente.Nombre = FString::Printf(TEXT("Iturri_%s"), *CallesFuentes[i].Key.Left(12));
            Fuente.Calle = CallesFuentes[i].Key;
            Fuente.Barrio = CallesFuentes[i].Value;
            Fuente.Posicion = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(Fuente.Barrio));
            Fuente.Radio = 100.0f;
            Fuente.AlturaCazoleta = 80.0f;
            Fuente.bFuncional = true;
            Fuentes.Add(Fuente);
        }
    }

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Placed = 0;
    for (const FRealFountain& Fuente : Fuentes)
    {
        FVector Pos = Fuente.Posicion;
        Pos.Z += Fuente.AlturaCazoleta;

        AStaticMeshActor* Cazoleta = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Pos, FRotator::ZeroRotator);
        if (!Cazoleta) continue;

        Cazoleta->SetMobility(EComponentMobility::Static);

        float Scale = Fuente.Radio / 50.0f;
        Cazoleta->SetActorScale3D(FVector(Scale, Scale, 0.3f));

        UStaticMesh* CylinderMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/EngineMeshes/Cylinder"));
        if (CylinderMesh)
            Cazoleta->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);

        AStaticMeshActor* Base = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Fuente.Posicion, FRotator::ZeroRotator);
        if (Base)
        {
            Base->SetMobility(EComponentMobility::Static);
            Base->SetActorScale3D(FVector(Scale * 1.2f, Scale * 1.2f, 0.15f));
            if (CylinderMesh)
                Base->GetStaticMeshComponent()->SetStaticMesh(CylinderMesh);
        }

#if WITH_EDITOR
        Cazoleta->SetActorLabel(*FString::Printf(TEXT("Fuente_%s"), *Fuente.Nombre.Left(20)));
        if (Base) Base->SetActorLabel(*FString::Printf(TEXT("FuenteBase_%s"), *Fuente.Nombre.Left(15)));
#endif

        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Fountains: %d fuentes reales colocadas"), Placed);
    return Placed;
}
