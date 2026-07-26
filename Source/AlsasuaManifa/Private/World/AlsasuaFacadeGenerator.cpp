#include "World/AlsasuaFacadeGenerator.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Components/BoxComponent.h"
#include "UObject/ConstructorHelpers.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaFacadeGenerator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarFachadas();
}

void UAlsasuaFacadeGenerator::Deinitialize()
{
    Fachadas.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaFacadeGenerator::CargarFachadas()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/building_facades.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("FacadeGenerator: No se pudo cargar building_facades.json"));
        return false;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid())
    {
        UE_LOG(LogTemp, Error, TEXT("FacadeGenerator: JSON invalido"));
        return false;
    }

    const TArray<TSharedPtr<FJsonValue>>* Arr;
    if (!Root->TryGetArrayField(TEXT(""), Arr))
    {
        UE_LOG(LogTemp, Error, TEXT("FacadeGenerator: No se encontro array raiz"));
        return false;
    }

    Fachadas.Empty(Arr->Num());
    for (const auto& Val : *Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        FBuildingFacadeEntry Entry;
        Entry.BuildingId = Obj->GetIntegerField(TEXT("building_id"));
        Entry.Barrio = Obj->GetStringField(TEXT("barrio"));
        Entry.MaterialFachada = Obj->GetStringField(TEXT("material_fachada"));

        const TArray<TSharedPtr<FJsonValue>>* ColArr;
        if (Obj->TryGetArrayField(TEXT("color_fachada"), ColArr))
        {
            for (const auto& C : *ColArr)
                Entry.ColorFachada.Add(C->AsNumber());
        }

        Entry.Estilo = Obj->GetStringField(TEXT("estilo"));
        Entry.NumNiveles = Obj->GetIntegerField(TEXT("num_niveles"));
        Entry.AlturaTotal = Obj->GetNumberField(TEXT("altura_total"));
        Entry.AlturaPorNivel = Obj->GetNumberField(TEXT("altura_por_nivel"));
        Entry.PerimetroAprox = Obj->GetNumberField(TEXT("perimetro_aprox"));
        Entry.AreaAprox = Obj->GetNumberField(TEXT("area_aprox"));
        Entry.MaterialTejado = Obj->GetStringField(TEXT("material_tejado"));

        const TArray<TSharedPtr<FJsonValue>>* ColTejado;
        if (Obj->TryGetArrayField(TEXT("color_tejado"), ColTejado))
        {
            for (const auto& C : *ColTejado)
                Entry.ColorTejado.Add(C->AsNumber());
        }

        const TArray<TSharedPtr<FJsonValue>>* VentArr;
        if (Obj->TryGetArrayField(TEXT("ventanas"), VentArr))
        {
            for (const auto& V : *VentArr)
            {
                const TSharedPtr<FJsonObject>& VO = V->AsObject();
                if (!VO) continue;
                FWindowData W;
                W.Tipo = VO->GetStringField(TEXT("tipo"));
                W.Ancho = VO->GetNumberField(TEXT("ancho"));
                W.Alto = VO->GetNumberField(TEXT("alto"));
                W.MaterialMarcos = VO->GetStringField(TEXT("material_marcos"));
                W.ColorMarcos = VO->GetStringField(TEXT("color_marcos"));
                W.bConPersiana = VO->GetBoolField(TEXT("con_persiana"));
                W.bConBalcon = VO->GetBoolField(TEXT("con_balcon"));
                Entry.Ventanas.Add(W);
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* BalArr;
        if (Obj->TryGetArrayField(TEXT("balcones"), BalArr))
        {
            for (const auto& B : *BalArr)
            {
                const TSharedPtr<FJsonObject>& BO = B->AsObject();
                if (!BO) continue;
                FBalconData Bal;
                Bal.Tipo = BO->GetStringField(TEXT("tipo"));
                Bal.Ancho = BO->GetNumberField(TEXT("ancho"));
                Bal.Profundidad = BO->GetNumberField(TEXT("profundidad"));
                Bal.Barandilla = BO->GetStringField(TEXT("barandilla"));
                Entry.Balcones.Add(Bal);
            }
        }

        const TArray<TSharedPtr<FJsonValue>>* TieArr;
        if (Obj->TryGetArrayField(TEXT("tiendas_planta_baja"), TieArr))
        {
            for (const auto& T : *TieArr)
            {
                const TSharedPtr<FJsonObject>& TO = T->AsObject();
                if (!TO) continue;
                FTiendaData Tie;
                Tie.Nombre = TO->GetStringField(TEXT("nombre"));
                Tie.Tipo = TO->GetStringField(TEXT("tipo"));
                Tie.AnchoM = TO->GetNumberField(TEXT("ancho_m"));
                Tie.AlturaM = TO->GetNumberField(TEXT("altura_m"));
                Tie.MaterialFachada = TO->GetStringField(TEXT("material_fachada"));
                Tie.bConToldo = TO->GetBoolField(TEXT("con_toldo"));
                Tie.ColorToldo = TO->GetStringField(TEXT("color_toldo"));
                Entry.TiendasPlantaBaja.Add(Tie);
            }
        }

        Fachadas.Add(Entry);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("FacadeGenerator: %d fachadas cargadas de building_facades.json"), Fachadas.Num());
    return true;
}

bool UAlsasuaFacadeGenerator::CargarEdificios()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/buildings_final.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("FacadeGenerator: No se pudo cargar buildings_final.json"));
        return false;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

    const TArray<TSharedPtr<FJsonValue>>* Arr;
    if (!Root->TryGetArrayField(TEXT(""), Arr)) return false;

    EdificiosCentros.Empty(Arr->Num());
    for (const auto& Val : *Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        int32 Id = Obj->GetIntegerField(TEXT("id"));
        const TArray<TSharedPtr<FJsonValue>>* Vs;
        if (!Obj->TryGetArrayField(TEXT("vertices"), Vs) || !Vs || Vs->Num() < 3) continue;

        FVector Centro(0, 0, 0);
        for (const auto& Pv : *Vs)
        {
            const TSharedPtr<FJsonObject> Po = Pv->AsObject();
            if (!Po) continue;
            const double ux = Po->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX;
            const double uz = Po->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ;
            const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector(ux, 0.0, uz));
            Centro += M;
        }
        Centro /= Vs->Num();

        const double AlturaM = Obj->HasField(TEXT("height")) ? Obj->GetNumberField(TEXT("height")) : 6.0;
        EdificiosCentros.Add(Id, FFacadeBuildingInfo(Centro, (float)AlturaM));
    }

    UE_LOG(LogTemp, Log, TEXT("FacadeGenerator: %d centros de edificios calculados"), EdificiosCentros.Num());
    return true;
}

int32 UAlsasuaFacadeGenerator::GenerarFachadasEnMundo()
{
    if (!bCargado)
    {
        UE_LOG(LogTemp, Warning, TEXT("FacadeGenerator: Datos no cargados, llamando CargarFachadas()"));
        if (!CargarFachadas()) return 0;
    }

    if (!EdificiosCentros.Num())
    {
        CargarEdificios();
    }

    UWorld* World = GetWorld();
    if (!World) return 0;

    int32 FacadesGenerated = 0;
    for (const FBuildingFacadeEntry& Fachada : Fachadas)
    {
        FFacadeBuildingInfo* Info = EdificiosCentros.Find(Fachada.BuildingId);
        if (!Info) continue;

        const FVector BaseLoc = Info->Centro;
        BaseLoc.Z += Info->AlturaM * 50.0f;

        int32 VentanasPorNivel = FMath::Max(2, (int32)(Fachada.PerimetroAprox / 3.0f));
        float Espaciado = Fachada.PerimetroAprox / (float)VentanasPorNivel;

        for (int32 Nivel = 0; Nivel < Fachada.NumNiveles; Nivel++)
        {
            float ZOffset = (Nivel + 0.5f) * Fachada.AlturaPorNivel * 100.0f;

            for (int32 V = 0; V < VentanasPorNivel && V < Fachada.Ventanas.Num(); V++)
            {
                float XOffset = (V - VentanasPorNivel / 2.0f) * Espaciado * 100.0f;
                FVector VentanaPos = BaseLoc + FVector(XOffset, 0, ZOffset);

                CrearVentanaProcedural(nullptr, Fachada.Ventanas[V % Fachada.Ventanas.Num()],
                    VentanaPos, FRotator::ZeroRotator, 1.0f);
            }
        }

        for (const FTiendaData& Tienda : Fachada.TiendasPlantaBaja)
        {
            FVector TiendaPos = BaseLoc + FVector(0, 0, Tienda.AlturaM * 50.0f * 100.0f);
            CrearTiendaProcedural(nullptr, Tienda, TiendaPos, FRotator::ZeroRotator);
        }

        FacadesGenerated++;
    }

    UE_LOG(LogTemp, Log, TEXT("FacadeGenerator: %d fachadas generadas en el mundo"), FacadesGenerated);
    return FacadesGenerated;
}

void UAlsasuaFacadeGenerator::CrearVentanaProcedural(AActor* Owner, const FWindowData& Ventana,
    const FVector& Pos, const FRotator& Rot, float Escala)
{
    UWorld* World = GetWorld();
    if (!World) return;

    AStaticMeshActor* VentanaActor = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Pos, Rot);
    if (!VentanaActor) return;

    VentanaActor->SetMobility(EComponentMobility::Movable);
    UStaticMeshComponent* Mesh = VentanaActor->GetStaticMeshComponent();
    if (Mesh)
    {
        Mesh->SetStaticMesh(nullptr);
        Mesh->SetMobility(EComponentMobility::Movable);
    }

#if WITH_EDITOR
    VentanaActor->SetActorLabel(*FString::Printf(TEXT("Ventana_%s"), *Ventana.Tipo));
#endif
}

void UAlsasuaFacadeGenerator::CrearBalconProcedural(AActor* Owner, const FBalconData& Balcon,
    const FVector& Pos, const FRotator& Rot)
{
    UWorld* World = GetWorld();
    if (!World) return;

    AStaticMeshActor* BalconActor = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Pos, Rot);
    if (!BalconActor) return;

#if WITH_EDITOR
    BalconActor->SetActorLabel(*FString::Printf(TEXT("Balcon_%s"), *Balcon.Tipo));
#endif
}

void UAlsasuaFacadeGenerator::CrearTiendaProcedural(AActor* Owner, const FTiendaData& Tienda,
    const FVector& Pos, const FRotator& Rot)
{
    UWorld* World = GetWorld();
    if (!World) return;

    AStaticMeshActor* TiendaActor = World->SpawnActor<AStaticMeshActor>(
        AStaticMeshActor::StaticClass(), Pos, Rot);
    if (!TiendaActor) return;

#if WITH_EDITOR
    TiendaActor->SetActorLabel(*FString::Printf(TEXT("Tienda_%s"), *Tienda.Nombre));
#endif
}
