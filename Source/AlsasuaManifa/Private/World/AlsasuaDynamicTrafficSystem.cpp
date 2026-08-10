#include "World/AlsasuaDynamicTrafficSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "GeoDataAlsasua.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UAlsasuaDynamicTrafficSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    bInicializado = true;
    // El callejero se carga al usarlo, no aquí: en Initialize de un
    // subsistema de GameInstance todavía no hay terreno, y los puntos de
    // ruta se quedarían todos a la cota de la plaza en vez de sobre su calle.
}

void UAlsasuaDynamicTrafficSystem::CargarCallejero()
{
    CallesDisponibles.Empty();

    TArray<FString> Lineas;
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Warning, TEXT("DynamicTraffic: No se pudo cargar roads_unity.json"));
        return;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TArray<TSharedPtr<FJsonValue>> RoadsArr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, RoadsArr))
    {
        UE_LOG(LogTemp, Warning, TEXT("DynamicTraffic: Error parseando roads_unity.json"));
        return;
    }

    for (const auto& RoadVal : RoadsArr)
    {
        const TSharedPtr<FJsonObject>& Road = RoadVal->AsObject();
        if (!Road) continue;

        FString RoadType;
        Road->TryGetStringField(TEXT("type"), RoadType);

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Road->TryGetArrayField(TEXT("points"), PointsArr)) continue;
        if (PointsArr->Num() < 4) continue;

        TArray<FVector> PuntosCalle;
        for (int32 i = 0; i < PointsArr->Num(); i++)
        {
            const TSharedPtr<FJsonObject>& Pt = (*PointsArr)[i]->AsObject();
            if (!Pt) continue;
            PuntosCalle.Add(UAlsasuaGeoData::RelLocalASueloUE5(GetWorld(),
                FVector(Pt->GetNumberField(TEXT("x")), 0.0f, Pt->GetNumberField(TEXT("z")))));
        }

        if (PuntosCalle.Num() >= 2)
        {
            CallesDisponibles.Add(PuntosCalle);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("DynamicTraffic: %d calles cargadas para rutas vehiculares"), CallesDisponibles.Num());
}

void UAlsasuaDynamicTrafficSystem::IniciarTrafico()
{
    if (CallesDisponibles.Num() == 0) CargarCallejero();

    if (!bInicializado) return;

    Vehiculos.Empty();

    for (int32 i = 0; i < MaxVehiculos; i++)
    {
        SpawnVehiculoEnCalle();
    }

    UE_LOG(LogTemp, Log, TEXT("DynamicTraffic: %d vehículos en circulación"), Vehiculos.Num());
}

void UAlsasuaDynamicTrafficSystem::ActualizarTrafico(float DeltaTime)
{
    TiempoDesdeUltimoSpawn += DeltaTime;

    if (TiempoDesdeUltimoSpawn >= FrecuenciaSpawn && Vehiculos.Num() < MaxVehiculos)
    {
        SpawnVehiculoEnCalle();
        TiempoDesdeUltimoSpawn = 0.0f;
    }

    for (FVehiclePath& Veh : Vehiculos)
    {
        if (!Veh.bEnMarcha || Veh.Puntos.Num() < 2) continue;

        FVector PosActual = (Veh.ActorAsociado.IsValid()) ?
            Veh.ActorAsociado->GetActorLocation() : Veh.Puntos[Veh.IndiceActual];

        FVector PosSiguiente = Veh.Puntos[FMath::Min(Veh.IndiceActual + 1, Veh.Puntos.Num() - 1)];
        FVector Direccion = (PosSiguiente - PosActual).GetSafeNormal();

        FVector NuevaPos = PosActual + Direccion * Veh.Velocidad * DeltaTime;

        float DistToNext = FVector::Distance(NuevaPos, PosSiguiente);
        if (DistToNext < 300.0f)
        {
            Veh.IndiceActual++;
            if (Veh.IndiceActual >= Veh.Puntos.Num() - 1)
            {
                Veh.IndiceActual = 0;
                NuevaPos = Veh.Puntos[0];
            }
        }

        if (Veh.ActorAsociado.IsValid())
        {
            Veh.ActorAsociado->SetActorLocation(NuevaPos);
            FRotator LookAt = Direccion.Rotation();
            Veh.ActorAsociado->SetActorRotation(LookAt);
        }
    }
}

void UAlsasuaDynamicTrafficSystem::SpawnVehiculoEnCalle()
{
    UWorld* World = GetWorld();
    if (!World) return;

    if (CallesDisponibles.Num() == 0) return;

    const int32 CalleIdx = FMath::RandRange(0, CallesDisponibles.Num() - 1);
    const TArray<FVector>& Calle = CallesDisponibles[CalleIdx];

    const int32 PuntoIdx = FMath::RandRange(0, FMath::Max(0, Calle.Num() - 2));
    FVector PuntoInicio = Calle[PuntoIdx];
    FVector PuntoFinal = Calle[FMath::Min(PuntoIdx + 1, Calle.Num() - 1)];

    FVehiclePath Veh;
    Veh.Tipo = static_cast<ETipoVehiculo>(FMath::RandRange(0, 2));
    Veh.Velocidad = (Veh.Tipo == ETipoVehiculo::Camion) ?
        FMath::RandRange(200.0f, 400.0f) : FMath::RandRange(300.0f, 700.0f);
    Veh.ColorCarroceria = ObtenerColorAleatorio();
    Veh.bEnMarcha = true;

    FVector Dir = (PuntoFinal - PuntoInicio).GetSafeNormal();
    FVector PerpDir = FVector(-Dir.Y, Dir.X, 0);
    FVector OffsetLateral = PerpDir * FMath::RandRange(-60.0f, 60.0f);

    int32 NumPuntos = FMath::Min(8, Calle.Num() - PuntoIdx);
    for (int32 i = 0; i < NumPuntos; i++)
    {
        int32 PtIdx = FMath::Min(PuntoIdx + i, Calle.Num() - 1);
        Veh.Puntos.Add(Calle[PtIdx] + OffsetLateral);
    }

    AStaticMeshActor* VehActor = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), PuntoInicio + OffsetLateral, Dir.Rotation());
    if (!VehActor) return;

    VehActor->SetMobility(EComponentMobility::Movable);

    UStaticMesh* CubeMesh = LoadObject<UStaticMesh>(nullptr,
        TEXT("/Engine/BasicShapes/Cube.Cube"));
    if (CubeMesh)
        VehActor->GetStaticMeshComponent()->SetStaticMesh(CubeMesh);

    float ScaleX, ScaleY, ScaleZ;
    if (Veh.Tipo == ETipoVehiculo::Camion)
    {
        ScaleX = 8.0f; ScaleY = 2.5f; ScaleZ = 2.8f;
    }
    else if (Veh.Tipo == ETipoVehiculo::Furgoneta)
    {
        ScaleX = 5.5f; ScaleY = 2.0f; ScaleZ = 2.2f;
    }
    else
    {
        ScaleX = 4.5f; ScaleY = 1.8f; ScaleZ = 1.5f;
    }
    VehActor->SetActorScale3D(FVector(ScaleX, ScaleY, ScaleZ));

    UMaterialInterface* BaseMat = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materiales/M_Vehiculo_Base"));
    if (!BaseMat)
    {
        BaseMat = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial"));
    }

    if (BaseMat)
    {
        UMaterialInstanceDynamic* DynMat = UMaterialInstanceDynamic::Create(BaseMat, VehActor);
        if (DynMat)
        {
            DynMat->SetVectorParameterValue(FName("Color"), Veh.ColorCarroceria);
            VehActor->GetStaticMeshComponent()->SetMaterial(0, DynMat);
        }
    }

    if (Veh.Tipo == ETipoVehiculo::Furgoneta)
    {
        UMaterialInterface* WhiteMat = LoadObject<UMaterialInterface>(nullptr,
            TEXT("/Game/Materiales/M_Vehiculo_Blanco"));
        if (WhiteMat)
            VehActor->GetStaticMeshComponent()->SetMaterial(0, WhiteMat);
    }

#if WITH_EDITOR
    const FString TipoStr = (Veh.Tipo == ETipoVehiculo::Camion) ? TEXT("Camion") :
        (Veh.Tipo == ETipoVehiculo::Furgoneta) ? TEXT("Furgoneta") : TEXT("Coche");
    VehActor->SetActorLabel(*FString::Printf(TEXT("Vehiculo_%s_%d"), *TipoStr, Vehiculos.Num()));
#endif

    Veh.ActorAsociado = VehActor;
    Vehiculos.Add(Veh);
}

FLinearColor UAlsasuaDynamicTrafficSystem::ObtenerColorAleatorio() const
{
    const TArray<FLinearColor> Colores = {
        FLinearColor(0.8f, 0.1f, 0.1f),
        FLinearColor(0.1f, 0.1f, 0.8f),
        FLinearColor(0.95f, 0.95f, 0.95f),
        FLinearColor(0.1f, 0.1f, 0.1f),
        FLinearColor(0.5f, 0.5f, 0.5f),
        FLinearColor(0.2f, 0.6f, 0.2f),
        FLinearColor(0.9f, 0.8f, 0.1f),
        FLinearColor(0.6f, 0.3f, 0.1f),
    };
    return Colores[FMath::RandRange(0, Colores.Num() - 1)];
}

FVector UAlsasuaDynamicTrafficSystem::ObtenerPuntoInicio() const
{
    if (CallesDisponibles.Num() > 0)
    {
        const int32 Idx = FMath::RandRange(0, CallesDisponibles.Num() - 1);
        const TArray<FVector>& Calle = CallesDisponibles[Idx];
        if (Calle.Num() > 0)
            return Calle[0];
    }

    FVector P = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(TEXT("Herriko")));
    P.Z = UAlsasuaGeoData::AlturaSueloUE5(GetWorld(), P.X, P.Y);
    return P;
}
