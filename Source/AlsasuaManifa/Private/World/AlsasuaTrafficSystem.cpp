#include "World/AlsasuaTrafficSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "World/AlsasuaMallaFab.h"

void UAlsasuaTrafficSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    // Los coches y las señales se calculan al colocarlos, no aquí: en
    // Initialize todavía no hay terreno sobre el que apoyarlos.
}

void UAlsasuaTrafficSystem::Deinitialize()
{
    Coches.Empty();
    SenalesTrafico.Empty();
    Super::Deinitialize();
}

void UAlsasuaTrafficSystem::GenerarCochesDesdeCalles()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath)) return;

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

    const TArray<TSharedPtr<FJsonValue>>* Arr;
    if (!Root->TryGetArrayField(TEXT("roads"), Arr) && !Root->TryGetArrayField(TEXT(""), Arr)) return;

    FRandomStream Rng(12345);
    const FString Colors[] = {TEXT("blanco"), TEXT("negro"), TEXT("gris"), TEXT("rojo"), TEXT("azul"),
        TEXT("plata"), TEXT("azul_oscuro"), TEXT("verde")};
    const int32 NumColors = 8;

    for (const auto& Val : *Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const FString Tipo = Obj->HasField(TEXT("type")) ? Obj->GetStringField(TEXT("type")) : TEXT("residential");
        const float AnchoVia = Obj->HasField(TEXT("width")) ? Obj->GetNumberField(TEXT("width")) : 8.0f;

        if (Tipo == TEXT("pedestrian") || Tipo == TEXT("path") || Tipo == TEXT("footway")) continue;
        if (AnchoVia < 5.0f) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Obj->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() == 0) continue;
        const TSharedPtr<FJsonObject>& FirstPt = (*PointsArr)[0]->AsObject();
        if (!FirstPt) continue;
        const float RawX = FirstPt->GetNumberField(TEXT("x"));
        const float RawZ = FirstPt->GetNumberField(TEXT("z"));
        const float X = RawX + UAlsasuaGeoData::OX;
        const float Z = RawZ + UAlsasuaGeoData::OZ;

        FVector Loc = UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(), FVector(RawX, 0.0f, RawZ));

        int32 NumCoches = (AnchoVia > 10.0f) ? 2 : 1;
        for (int32 i = 0; i < NumCoches; i++)
        {
            if (Rng.GetFraction() > 0.3f) continue;

            FParkedCar Car;
            Car.Calle = Obj->HasField(TEXT("name")) ? Obj->GetStringField(TEXT("name")) : TEXT("");
            Car.X = X + Rng.FRandRange(-2.0f, 2.0f);
            Car.Z = Z + Rng.FRandRange(-2.0f, 2.0f);
            Car.Rotacion = Rng.FRandRange(0.0f, 360.0f);
            Car.Color = Colors[Rng.RandRange(0, NumColors - 1)];
            Car.Tipo = (Rng.GetFraction() < 0.8f) ? TEXT("coche") : TEXT("furgoneta");

            Coches.Add(Car);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d coches aparcados generados"), Coches.Num());
}

void UAlsasuaTrafficSystem::GenerarSenalesDesdeCalles()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath)) return;

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return;

    const TArray<TSharedPtr<FJsonValue>>* Arr2;
    if (!Root->TryGetArrayField(TEXT("roads"), Arr2) && !Root->TryGetArrayField(TEXT(""), Arr2)) return;

    FRandomStream Rng(54321);

    for (const auto& Val : *Arr2)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const float AnchoVia = Obj->HasField(TEXT("width")) ? Obj->GetNumberField(TEXT("width")) : 8.0f;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Obj->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() == 0) continue;
        const TSharedPtr<FJsonObject>& FirstPt = (*PointsArr)[0]->AsObject();
        if (!FirstPt) continue;
        const float X = FirstPt->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX;
        const float Z = FirstPt->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ;

        if (AnchoVia > 8.0f && Rng.GetFraction() < 0.3f)
        {
            FTrafficSign Sign;
            Sign.Tipo = TEXT("ceda_el_paso");
            Sign.X = X;
            Sign.Z = Z;
            Sign.Rotacion = Rng.FRandRange(0.0f, 360.0f);
            SenalesTrafico.Add(Sign);
        }
        if (Rng.GetFraction() < 0.15f)
        {
            FTrafficSign Sign;
            Sign.Tipo = TEXT("velocidad_30");
            Sign.X = X + Rng.FRandRange(-1.0f, 1.0f);
            Sign.Z = Z + Rng.FRandRange(-1.0f, 1.0f);
            Sign.Rotacion = Rng.FRandRange(0.0f, 360.0f);
            SenalesTrafico.Add(Sign);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d señales de tráfico generadas"), SenalesTrafico.Num());
}

int32 UAlsasuaTrafficSystem::ColocarCocheAparcado()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    if (Coches.Num() == 0) GenerarCochesDesdeCalles();

    // Una sola resolución para todos los coches, fuera del bucle: antes se hacía un
    // LoadObject por coche de una ruta constante. Y sobre todo, esa ruta era
    // /Game/AssetsImportados/Casas/HousePack/House1 — cada coche aparcado del pueblo
    // era una CASA. Viene de arrastrar la ruta del sistema de casas; el comentario
    // que había sólo corregía la ruta, no que fuese el asset equivocado.
    // AlsasuaMallaFab busca un coche por palabra clave y cae a forma básica si no
    // hay ninguno, que es el patrón de degradación del proyecto.
    UStaticMesh* CocheMesh = AlsasuaMallaFab::Resolver(
        TEXT("coche"), TEXT("/Engine/BasicShapes/Cube.Cube"));

    int32 Placed = 0;
    for (const FParkedCar& Car : Coches)
    {
        FVector Loc = UAlsasuaGeoData::UnityaUnreal(FVector(Car.X, Car.Z, 0));

        AStaticMeshActor* CarActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0, Car.Rotacion, 0));
        if (CarActor)
        {
            CarActor->SetMobility(EComponentMobility::Movable);
            CarActor->SetActorScale3D(FVector(1.0f));

            if (CocheMesh) CarActor->GetStaticMeshComponent()->SetStaticMesh(CocheMesh);

#if WITH_EDITOR
            CarActor->SetActorLabel(*FString::Printf(TEXT("Coche_%s_%s"), *Car.Color, *Car.Tipo));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d coches aparcados"), Placed);
    return Placed;
}

int32 UAlsasuaTrafficSystem::ColocarSenalesTrafico()
{
    UWorld* World = GetWorld();
    if (!World) return 0;

    if (SenalesTrafico.Num() == 0) GenerarSenalesDesdeCalles();

    int32 Placed = 0;
    for (const FTrafficSign& Sign : SenalesTrafico)
    {
        FVector Loc = UAlsasuaGeoData::UnityaUnreal(FVector(Sign.X, Sign.Z, 0));
        Loc.Z += 200.0f;

        AStaticMeshActor* SignActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0, Sign.Rotacion, 0));
        if (SignActor)
        {
            SignActor->SetMobility(EComponentMobility::Movable);
#if WITH_EDITOR
            SignActor->SetActorLabel(*FString::Printf(TEXT("Trafico_%s"), *Sign.Tipo));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("TrafficSystem: %d señales de tráfico colocadas"), Placed);
    return Placed;
}
