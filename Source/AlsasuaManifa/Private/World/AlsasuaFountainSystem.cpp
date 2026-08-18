#include "World/AlsasuaFountainSystem.h"
#include "World/AlsasuaMallaFab.h"
#include "Engine/StaticMesh.h"
#include "Materials/MaterialInterface.h"
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
                // Las 5 fuentes del dataset están en absoluto, pero
                // street_furniture mezcla marcos: MobiliarioAUE5 lo decide por
                // pieza y así no depende de que sigan siendo todas del mismo.
                Fuente.Posicion = UAlsasuaGeoData::MobiliarioAUE5(FVector(X, 0.0f, Z));
                Fuente.Posicion.Z = UAlsasuaGeoData::AlturaSueloUE5(GetWorld(),
                    Fuente.Posicion.X, Fuente.Posicion.Y);
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

            // Las dos de Herriko caían en el mismo punto exacto, una dentro de
            // otra: BarrioCenter da un punto por barrio, no por fuente. Un
            // desplazamiento fijo por índice las separa sin volverlas aleatorias.
            // Es el respaldo de cuando falta street_furniture.json; con el
            // fichero, las 5 vienen con coordenada propia.
            const float Angulo = i * 137.5f;   // ángulo áureo: no se repite en 5
            Fuente.Posicion.X += 1500.0f * FMath::Cos(FMath::DegreesToRadians(Angulo));
            Fuente.Posicion.Y += 1500.0f * FMath::Sin(FMath::DegreesToRadians(Angulo));

            Fuente.Posicion.Z = UAlsasuaGeoData::AlturaSueloUE5(GetWorld(),
                Fuente.Posicion.X, Fuente.Posicion.Y);
            Fuente.Radio = 100.0f;
            Fuente.AlturaCazoleta = 80.0f;
            Fuente.bFuncional = true;
            Fuentes.Add(Fuente);
        }
    }

    UWorld* World = GetWorld();
    if (!World) return 0;

    // Fuera del bucle: eran dos LoadObject por fuente. Y la ruta era
    // /Engine/EngineMeshes/Cylinder, que no es la de las formas básicas: si no
    // resuelve, los dos actores de cada fuente se quedan sin malla y las cinco
    // fuentes salen invisibles sin una línea en el log. Por AlsasuaMallaFab
    // entra además la fuente de verdad si está descargada.
    UStaticMesh* MallaFuente = AlsasuaMallaFab::Resolver(TEXT("fuente"),
        TEXT("/Engine/BasicShapes/Cylinder.Cylinder"));
    if (!MallaFuente)
    {
        UE_LOG(LogTemp, Warning, TEXT("Fountains: sin malla; no se colocan las %d fuentes."), Fuentes.Num());
        return 0;
    }
    UMaterialInterface* MatPiedra = LoadObject<UMaterialInterface>(nullptr,
        TEXT("/Game/Materiales/M_Piedra"));

    int32 Placed = 0;
    for (const FRealFountain& Fuente : Fuentes)
    {
        // El cilindro del motor mide 100 uu de alto y se centra en su origen.
        //
        // Antes el pie iba a cota de suelo con escala 0.15 —o sea 15 cm de alto,
        // medio enterrado— y la cazoleta a suelo+80 con escala 0.3, o sea
        // flotando con su base a 65 cm: 57 cm de aire entre las dos piezas. Una
        // fuente de calle es un pie que llega hasta la cazoleta.
        const float Escala = Fuente.Radio / 50.0f;
        const float AltoPie = Fuente.AlturaCazoleta;          // cm
        const float AltoCazoleta = 30.0f;

        AStaticMeshActor* Pie = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(),
            Fuente.Posicion + FVector(0.0f, 0.0f, AltoPie * 0.5f), FRotator::ZeroRotator);
        if (!Pie) continue;
        Pie->SetMobility(EComponentMobility::Static);
        Pie->SetActorScale3D(FVector(Escala * 0.45f, Escala * 0.45f, AltoPie / 100.0f));
        Pie->GetStaticMeshComponent()->SetStaticMesh(MallaFuente);
        if (MatPiedra) Pie->GetStaticMeshComponent()->SetMaterial(0, MatPiedra);

        AStaticMeshActor* Cazoleta = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(),
            Fuente.Posicion + FVector(0.0f, 0.0f, AltoPie + AltoCazoleta * 0.5f),
            FRotator::ZeroRotator);
        if (Cazoleta)
        {
            Cazoleta->SetMobility(EComponentMobility::Static);
            Cazoleta->SetActorScale3D(FVector(Escala, Escala, AltoCazoleta / 100.0f));
            Cazoleta->GetStaticMeshComponent()->SetStaticMesh(MallaFuente);
            if (MatPiedra) Cazoleta->GetStaticMeshComponent()->SetMaterial(0, MatPiedra);
        }

#if WITH_EDITOR
        Pie->SetActorLabel(*FString::Printf(TEXT("FuentePie_%s"), *Fuente.Nombre.Left(15)));
        if (Cazoleta) Cazoleta->SetActorLabel(*FString::Printf(TEXT("Fuente_%s"), *Fuente.Nombre.Left(20)));
#endif

        Placed++;
    }

    UE_LOG(LogTemp, Log, TEXT("Fountains: %d fuentes reales colocadas"), Placed);
    return Placed;
}
