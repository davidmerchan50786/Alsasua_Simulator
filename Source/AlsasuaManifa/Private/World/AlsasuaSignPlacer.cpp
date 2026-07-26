#include "World/AlsasuaSignPlacer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
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

    int32 Placed = 0;
    for (const FSignEntry& S : Senales)
    {
        FVector Loc = UAlsasuaGeoData::UnityaUnreal(FVector(S.X, S.Z, 0));
        Loc.Z += S.AlturaM * 100.0f;

        AStaticMeshActor* SignActor = World->SpawnActor<AStaticMeshActor>(
            AStaticMeshActor::StaticClass(), Loc, FRotator::ZeroRotator);
        if (SignActor)
        {
            SignActor->SetMobility(EComponentMobility::Movable);
#if WITH_EDITOR
            SignActor->SetActorLabel(*FString::Printf(TEXT("Sign_%s_%s"), *S.Tipo, *S.Texto));
#endif
            Placed++;
        }
    }

    UE_LOG(LogTemp, Log, TEXT("SignPlacer: %d senales colocadas en el mundo"), Placed);
    return Placed;
}
