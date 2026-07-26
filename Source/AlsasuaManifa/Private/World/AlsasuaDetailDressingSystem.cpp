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

int32 UAlsasuaDetailDressingSystem::ColocarDetalle()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Total = 0;

    ColocarMacetas(World);
    ColocarBuzones(World);
    ColocarPapeleiras(World);
    ColocarBancos(World);
    ColocarVallasVerdes(World);

    Total = Detalles.Num();
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
        FVector Centro;
        if (Barrio == TEXT("Herriko"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f, 0.0f, 8568.5f));
        else if (Barrio == TEXT("Zelai"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f, 0.0f, 8573.5f));
        else if (Barrio == TEXT("SanPedro"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1895.2f, 0.0f, 8565.5f));
        else
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1889.5f, 0.0f, 8569.0f));

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
        FVector Centro;
        if (Barrio == TEXT("Herriko"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1891.5f, 0.0f, 8568.5f));
        else if (Barrio == TEXT("Zelai"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f, 0.0f, 8573.5f));
        else if (Barrio == TEXT("Intxostia"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1890.0f, 0.0f, 8577.0f));
        else if (Barrio == TEXT("SanPedro"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1895.2f, 0.0f, 8565.5f));
        else if (Barrio == TEXT("Errota"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1897.0f, 0.0f, 8570.5f));
        else if (Barrio == TEXT("Harrobieta"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1889.5f, 0.0f, 8569.0f));
        else
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1892.5f, 0.0f, 8571.5f));

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

        FVector Centro;
        if (Barrio == TEXT("Zelai"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1893.0f, 0.0f, 8573.5f));
        else if (Barrio == TEXT("Monte"))
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1894.0f, 0.0f, 8575.0f));
        else
            Centro = UAlsasuaGeoData::UnityaUnreal(FVector(1890.0f, 0.0f, 8577.0f));

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
