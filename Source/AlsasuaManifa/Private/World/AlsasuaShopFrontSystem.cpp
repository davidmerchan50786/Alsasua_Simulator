#include "World/AlsasuaShopFrontSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaShopFrontSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarTiendas();
}

void UAlsasuaShopFrontSystem::Deinitialize()
{
    Tiendas.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaShopFrontSystem::CargarTiendas()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/signage_data.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath)) return false;

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TArray<TSharedPtr<FJsonValue>> Arr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Arr)) return false;

    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const FString Tipo = Obj->GetStringField(TEXT("tipo"));
        if (Tipo != TEXT("señal_comercio") && Tipo != TEXT("comercial")) continue;

        FShopFront Shop;
        Shop.Nombre = Obj->GetStringField(TEXT("texto"));
        Shop.Tipo = TEXT("tienda");
        Shop.Barrio = Obj->HasField(TEXT("barrio")) ? Obj->GetStringField(TEXT("barrio")) : TEXT("Herriko");
        Shop.X = Obj->HasField(TEXT("x")) ? Obj->GetNumberField(TEXT("x")) : 1891.5;
        Shop.Z = Obj->HasField(TEXT("z")) ? Obj->GetNumberField(TEXT("z")) : 8572.0;
        Shop.Rotacion = FMath::FRandRange(0.0f, 360.0f);
        Shop.AnchoM = 4.0f;
        Shop.AlturaM = 3.0f;
        Shop.ColorFachada = TEXT("blanco");

        const FString LowerName = Shop.Nombre.ToLower();
        Shop.bConToldo = LowerName.Contains(TEXT("taberna")) || LowerName.Contains(TEXT("cafe"))
            || LowerName.Contains(TEXT("bar")) || LowerName.Contains(TEXT("pintxos"));
        Shop.ColorToldo = LowerName.Contains(TEXT("okindegia")) ? TEXT("blanco") : TEXT("rojo");
        Shop.bConRotulo = true;
        Shop.Horario = TEXT("10:00-14:00 / 17:00-21:00");

        Tiendas.Add(Shop);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("ShopFront: %d tiendas reales cargadas"), Tiendas.Num());
    return true;
}

int32 UAlsasuaShopFrontSystem::ColocarTiendasEnMundo()
{
    if (!bCargado && !CargarTiendas()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 Placed = 0;
    for (const FShopFront& Tienda : Tiendas)
    {
        FVector Loc = UAlsasuaGeoData::UnityaUnreal(FVector(Tienda.X, Tienda.Z, 0));

        AStaticMeshActor* ShopActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0, Tienda.Rotacion, 0));
        if (!ShopActor) continue;

        ShopActor->SetMobility(EComponentMobility::Movable);

        float ScaleX = Tienda.AnchoM * 100.0f;
        float ScaleZ = Tienda.AlturaM * 100.0f;
        ShopActor->SetActorScale3D(FVector(ScaleX / 100.0f, 1.0f, ScaleZ / 100.0f));

        UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
            TEXT("/Engine/BasicShapes/Cube.Cube"));
        if (CubeMesh)
            ShopActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

        UMaterialInterface* ShopMat = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materiales/M_Tienda"));
        if (ShopMat) ShopActor->GetStaticMeshComponent()->SetMaterial(0, ShopMat);

#if WITH_EDITOR
        ShopActor->SetActorLabel(*FString::Printf(TEXT("Tienda_%s_%s"),
            *Tienda.Nombre.Left(15), *Tienda.Tipo));
#endif
        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("ShopFront: %d tiendas reales colocadas"), Placed);
    return Placed;
}
