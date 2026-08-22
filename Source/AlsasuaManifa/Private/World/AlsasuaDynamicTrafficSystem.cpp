#include "World/AlsasuaDynamicTrafficSystem.h"
#include "World/AlsasuaRedViaria.h"
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
    UWorld* W = GetWorld();
    if (!W) return;

    // El callejero ya no se parsea aquí. Lo hacía este sistema por su cuenta, y
    // el de peatones otra vez, y salían dos listas de polilíneas sueltas: sin
    // cruces no hay por dónde girar. Además el 'type' se leía a una variable y
    // no se usaba —los coches circulaban por las 87 vías peatonales— y un
    // "Num() < 4" descartaba 192 de las 489 vías, el 39%, porque la mediana del
    // dataset es justo 4 puntos.
    Red = W->GetSubsystem<UAlsasuaRedViaria>();
    if (!Red) { UE_LOG(LogTemp, Warning, TEXT("DynamicTraffic: sin UAlsasuaRedViaria.")); return; }
    Red->Construir();
}

void UAlsasuaDynamicTrafficSystem::IniciarTrafico()
{
    if (!Red || !Red->EstaLista()) CargarCallejero();

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

    if (!Red || !Red->EstaLista()) return;

    for (FVehiclePath& Veh : Vehiculos)
    {
        if (!Veh.bEnMarcha || Veh.TramoActual < 0) continue;

        const FTramoViario& T = Red->Tramo(Veh.TramoActual);
        Veh.Avance += Veh.Velocidad * DeltaTime;

        // Al llegar al nodo se elige continuación en el cruce, en vez de saltar
        // al principio de la calle. SiguienteTramo evita la media vuelta salvo
        // en fondo de saco, que es lo que hace que parezca que circula.
        // Un tramo por frame: si el coche se pasara de largo varios tramos en un
        // solo frame, el sobrante se consume en el siguiente. A 700 cm/s y con
        // tramos de decenas de metros no llega a pasar.
        if (T.LargoCm > 0.f && Veh.Avance >= T.LargoCm)
        {
            const int32 Siguiente = Red->SiguienteTramo(Veh.TramoActual, Veh.Semilla++);
            if (Siguiente < 0) { Veh.bEnMarcha = false; continue; }
            Veh.Avance -= T.LargoCm;
            Veh.TramoActual = Siguiente;
        }

        const FTramoViario& Actual = Red->Tramo(Veh.TramoActual);
        const FVector P0 = Red->PosicionNodo(Actual.NodoA);
        const FVector P1 = Red->PosicionNodo(Actual.NodoB);
        const FVector Dir = (P1 - P0).GetSafeNormal();
        const float Fraccion = (Actual.LargoCm > 0.f)
            ? FMath::Clamp(Veh.Avance / Actual.LargoCm, 0.f, 1.f) : 0.f;

        // Carril: desplazado a la derecha de la marcha un cuarto del ancho de
        // calzada (lo fija el spawn). Antes era un ±60 cm sorteado, igual en una
        // pista de servicio que en la autovía.
        const FVector Perp(-Dir.Y, Dir.X, 0.f);
        const FVector NuevaPos = FMath::Lerp(P0, P1, Fraccion) + Perp * Veh.CarrilCm;

        if (Veh.ActorAsociado.IsValid())
        {
            Veh.ActorAsociado->SetActorLocation(NuevaPos);
            Veh.ActorAsociado->SetActorRotation(Dir.Rotation());
        }
    }
}

void UAlsasuaDynamicTrafficSystem::SpawnVehiculoEnCalle()
{
    UWorld* World = GetWorld();
    if (!World) return;
    if (!Red || !Red->EstaLista()) return;

    FVehiclePath Veh;
    Veh.Semilla = Vehiculos.Num() * 7919 + 13;

    Veh.TramoActual = Red->TramoAleatorio(Veh.Semilla++);
    if (Veh.TramoActual < 0) return;

    const FTramoViario& T = Red->Tramo(Veh.TramoActual);
    const FVector PuntoInicio = Red->PosicionNodo(T.NodoA);
    const FVector PuntoFinal = Red->PosicionNodo(T.NodoB);

    FRandomStream Sorteo(Veh.Semilla++);
    Veh.Tipo = static_cast<ETipoVehiculo>(Sorteo.RandRange(0, 2));
    Veh.Velocidad = (Veh.Tipo == ETipoVehiculo::Camion) ?
        Sorteo.FRandRange(200.0f, 400.0f) : Sorteo.FRandRange(300.0f, 700.0f);
    Veh.ColorCarroceria = ObtenerColorAleatorio();
    Veh.bEnMarcha = true;
    Veh.Avance = 0.f;

    // Carril derecho: a un cuarto del ancho de la calzada del eje. Antes era un
    // ±60 cm sorteado, igual en una pista de servicio que en la autovía.
    Veh.CarrilCm = T.AnchoCm * 0.25f;

    const FVector Dir = (PuntoFinal - PuntoInicio).GetSafeNormal();
    const FVector PerpDir(-Dir.Y, Dir.X, 0.f);
    const FVector OffsetLateral = PerpDir * Veh.CarrilCm;

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
    if (Red && Red->EstaLista())
    {
        const int32 Idx = Red->TramoAleatorio(Vehiculos.Num() * 31 + 7);
        if (Idx >= 0) return Red->PosicionNodo(Red->Tramo(Idx).NodoA);
    }

    FVector P = UAlsasuaGeoData::AbsLocalToUE5(UAlsasuaGeoData::BarrioCenter(TEXT("Herriko")));
    P.Z = UAlsasuaGeoData::AlturaSueloUE5(GetWorld(), P.X, P.Y);
    return P;
}
