#include "World/AlsasuaSignPlacer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/StaticMesh.h"
#include "Components/StaticMeshComponent.h"
#include "World/AlsasuaMallaFab.h"
#include "AjusteMallaComun.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaSignPlacer::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarSenales();
}

void UAlsasuaSignPlacer::Deinitialize()
{
    Senales.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaSignPlacer::CargarSenales()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/signage_data.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("SignPlacer: No se pudo cargar signage_data.json"));
        return false;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TArray<TSharedPtr<FJsonValue>> Arr;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Arr)) return false;

    Senales.Empty(Arr.Num());
    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        FSignEntry S;
        S.Tipo = Obj->GetStringField(TEXT("tipo"));
        S.Texto = Obj->GetStringField(TEXT("texto"));

        if (Obj->HasField(TEXT("barrio"))) S.Barrio = Obj->GetStringField(TEXT("barrio"));
        if (Obj->HasField(TEXT("idioma"))) S.Idioma = Obj->GetStringField(TEXT("idioma"));
        if (Obj->HasField(TEXT("x"))) S.X = Obj->GetNumberField(TEXT("x"));
        if (Obj->HasField(TEXT("z"))) S.Z = Obj->GetNumberField(TEXT("z"));
        if (Obj->HasField(TEXT("ancho_m"))) S.AnchoM = Obj->GetNumberField(TEXT("ancho_m"));
        if (Obj->HasField(TEXT("alto_m"))) S.AltoM = Obj->GetNumberField(TEXT("alto_m"));
        if (Obj->HasField(TEXT("altura_m"))) S.AlturaM = Obj->GetNumberField(TEXT("altura_m"));
        if (Obj->HasField(TEXT("material"))) S.Material = Obj->GetStringField(TEXT("material"));
        if (Obj->HasField(TEXT("bilingue"))) S.bBilingue = Obj->GetBoolField(TEXT("bilingue"));
        if (Obj->HasField(TEXT("con_luz"))) S.bConLuz = Obj->GetBoolField(TEXT("con_luz"));
        if (Obj->HasField(TEXT("tipo_negocio"))) S.TipoNegocio = Obj->GetStringField(TEXT("tipo_negocio"));

        Senales.Add(S);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("SignPlacer: %d senales cargadas"), Senales.Num());
    return true;
}

int32 UAlsasuaSignPlacer::ColocarSenalesEnMundo()
{
    if (!bCargado && !CargarSenales()) return 0;

    UWorld* World = GetWorld();
    if (!World) return 0;

    // Estos actores se creaban sin malla: aunque hubieran caído donde tocaba,
    // habrían sido actores vacíos e invisibles. Cada tipo de signage_data.json
    // resuelve el suyo por AlsasuaMallaFab, y lo que devuelva se ajusta a la
    // altura real de la pieza, que es la medida que manda en una señal.
    // Los seis tipos que trae de verdad signage_data.json, por frecuencia:
    // señal_comercio 56, señal_informativa 21, calle 20, señal_calle 19,
    // señal_transito 6, señal_turistica 4.
    auto MallaDe = [](const FString& Tipo) -> const TCHAR*
    {
        if (Tipo.Contains(TEXT("comercio")))  return TEXT("cartel_pared");
        if (Tipo.Contains(TEXT("transito")))  return TEXT("señal_velocidad");
        if (Tipo.Contains(TEXT("calle")))     return TEXT("placa_calle");
        return TEXT("cartel_pared");   // informativa y turística: panel
    };

    struct FModelo { UStaticMesh* Malla; AjusteMalla::FColocacion Col; };
    TMap<FString, FModelo> Cache;

    // 30 de las 126 señales de signage_data.json traen coordenadas basura: hasta
    // 123 km al oeste y 215 km al sur, todas de tipo señal_comercio o
    // señal_informativa, que son las que el generador situaba por dirección
    // cuando la encontraba. No hay forma de colocarlas bien —no se sabe dónde
    // van—, así que se dejan fuera y se dice cuántas: colocarlas estira los
    // límites del mundo y su trazo de suelo no encuentra terreno.
    // La caja del terreno vive en UAlsasuaGeoData; estaba copiada aquí a mano.
    int32 Placed = 0, Descartadas = 0;
    for (const FSignEntry& S : Senales)
    {
        if (!UAlsasuaGeoData::DentroDelTerreno(UAlsasuaGeoData::AbsLocalToUE5(FVector(S.X, 0.0, S.Z))))
        {
            ++Descartadas;
            continue;
        }

        const FString Clave = MallaDe(S.Tipo);
        FModelo& M = Cache.FindOrAdd(Clave);
        if (!M.Malla)
        {
            M.Malla = AlsasuaMallaFab::Resolver(*Clave, TEXT("/Engine/BasicShapes/Cube.Cube"));
            M.Col = AjusteMalla::Calcular(M.Malla,
                FVector(FMath::Max(0.2f, S.AnchoM), 0.12f, FMath::Max(0.2f, S.AltoM)),
                AlsasuaMallaFab::VieneDeFab(Clave), AjusteMalla::EEncaje::Alto);
        }
        if (!M.Col.bValido) continue;

        // Aquí se llamaba a UnityaUnreal(X, Z, 0), que mete la coordenada norte
        // en el eje vertical: las 126 señales salían sobre la línea norte=0 y
        // flotando a la altura de su coordenada norte. Y el "+= AlturaM" sumaba
        // sobre una Z que ya venía mal. signage_data.json es local ABSOLUTO.
        FVector Loc = UAlsasuaGeoData::AbsLocalToUE5(FVector(S.X, 0.0, S.Z));
        Loc.Z = UAlsasuaGeoData::AlturaSueloUE5(World, Loc.X, Loc.Y)
              + S.AlturaM * 100.0f + M.Col.SubirCm;

        AStaticMeshActor* SignActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator(0.f, M.Col.YawExtra, 0.f));
        if (SignActor)
        {
            SignActor->SetMobility(EComponentMobility::Movable);
            SignActor->SetActorScale3D(M.Col.Escala);
            SignActor->GetStaticMeshComponent()->SetStaticMesh(M.Malla);
#if WITH_EDITOR
            SignActor->SetActorLabel(*FString::Printf(TEXT("Sign_%s_%s"), *S.Tipo, *S.Texto));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log,
        TEXT("SignPlacer: %d señales colocadas, %d descartadas por caer fuera del terreno"),
        Placed, Descartadas);
    return Placed;
}
