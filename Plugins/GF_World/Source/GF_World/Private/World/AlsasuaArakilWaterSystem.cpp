#include "World/AlsasuaArakilWaterSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"

void UAlsasuaArakilWaterSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarTramosRio();
}

void UAlsasuaArakilWaterSystem::Deinitialize()
{
    Tramos.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaArakilWaterSystem::CargarTramosRio()
{
    // Por JsonDatos: hoy la raíz es un array, pero deserializar a TArray contra
    // una raíz de objeto devuelve false sin decir nada, y así estuvo muerta la
    // vía férrea entera. Ver CLAUDE.md §11.
    TArray<TSharedPtr<FJsonValue>> Rios;
    if (!JsonDatos::CargarArray(TEXT("Datos/waterways_unity.json"), Rios,
        { TEXT("waterways"), TEXT("rivers") }))
    {
        UE_LOG(LogTemp, Error, TEXT("ArakilWater: sin cauces en waterways_unity.json"));
        return false;
    }

    for (const TSharedPtr<FJsonValue>& RiverVal : Rios)
    {
        const TSharedPtr<FJsonObject> River = RiverVal->AsObject();
        if (!River.IsValid()) continue;

        // Un cauce del fichero viene sin nombre: GetStringField sobre un campo
        // que no está devuelve vacío, pero pedirlo con TryGet lo deja explícito.
        FString Name;
        River->TryGetStringField(TEXT("name"), Name);
        double AnchoM = 8.0;
        River->TryGetNumberField(TEXT("width"), AnchoM);
        const float Width = static_cast<float>(AnchoM);

        const TArray<TSharedPtr<FJsonValue>>* PtsArr;
        if (!River->TryGetArrayField(TEXT("pts"), PtsArr) || !PtsArr || PtsArr->Num() < 6) continue;

        TArray<FVector> Points;
        for (int32 i = 0; i + 2 < PtsArr->Num(); i += 3)
        {
            float PX = (*PtsArr)[i]->AsNumber();
            float PY = (*PtsArr)[i + 1]->AsNumber();
            float PZ = (*PtsArr)[i + 2]->AsNumber();
            // pts es plano [x,y,z,...], así que PY ya es la componente vertical:
            // el orden que espera UnityaUnreal. // ejes ok
            Points.Add(UAlsasuaGeoData::UnityaUnreal(FVector(PX, PY, PZ)));
        }

        for (int32 i = 0; i < Points.Num() - 1; i++)
        {
            const FVector& Loc0 = Points[i];
            const FVector& Loc1 = Points[i + 1];
            FVector Centro = (Loc0 + Loc1) * 0.5f;
            float Largo = FVector::Distance(Loc0, Loc1);

            // El ancho separa los dos cauces del valle —el Arakil y el
            // Altzania, 8 m— de los 71 regatas de monte, que van a 2. Un regato
            // de ladera no baja ni del mismo color ni a la misma velocidad, y
            // todos los tramos salían con los mismos parámetros.
            const bool bMayor = (Width >= 5.0f);

            FWaterSegment Seg;
            Seg.Centro = Centro;
            Seg.Nombre = Name;
            Seg.bCauceMayor = bMayor;
            Seg.Ancho = Width * 100.0f;
            Seg.Largo = Largo;
            Seg.Profundidad = bMayor ? 200.0f : 40.0f;
            // El regato corre más y hace más espuma: menos caudal, más pendiente.
            Seg.VelocidadFlujo = WaterSpeed * (bMayor ? 1.0f : 1.6f);
            Seg.ColorSuperficie = RiverColor;
            Seg.ColorProfundo = FLinearColor(RiverColor.R * 0.3f, RiverColor.G * 0.3f, RiverColor.B * 0.3f, 0.95f);
            Seg.ColorEspuma = FLinearColor(0.8f, 0.85f, 0.9f, FoamIntensity * (bMayor ? 1.0f : 1.5f));
            // El Arakil arrastra limo del valle; el regato baja limpio de roca.
            Seg.Turbidez = bMayor ? 0.35f : 0.12f;

            Tramos.Add(MoveTemp(Seg));
        }
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("ArakilWater: %d tramos del río Arakil cargados"), Tramos.Num());
    return true;
}

int32 UAlsasuaArakilWaterSystem::PublicarAgua()
{
    if (!bCargado && !CargarTramosRio()) return 0;

    // No se construye nada a propósito. Ver la cabecera: esto ponía un
    // AStaticMeshActor por tramo —2392— con un Plane escalado y SIN ROTAR,
    // encima del cauce que UCargadorVias ya drapea desde este mismo fichero.
    int32 Mayores = 0;
    for (const FWaterSegment& S : Tramos) if (S.bCauceMayor) ++Mayores;

    UE_LOG(LogTemp, Log,
        TEXT("ArakilWater: %d tramos caracterizados (%d de cauce mayor, %d de regato). No crea geometría: el cauce es de UCargadorVias."),
        Tramos.Num(), Mayores, Tramos.Num() - Mayores);
    return Tramos.Num();
}
