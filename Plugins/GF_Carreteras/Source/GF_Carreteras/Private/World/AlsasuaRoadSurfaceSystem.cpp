#include "CalleGenerada.h"
#include "EngineUtils.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "World/AlsasuaRoadSurfaceSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"

void UAlsasuaRoadSurfaceSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarSuperficies();
}

void UAlsasuaRoadSurfaceSystem::Deinitialize()
{
    Superficies.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaRoadSurfaceSystem::CargarSuperficies()
{
    const FString JsonPath = FPaths::ProjectContentDir() + TEXT("Datos/roads_unity.json");
    TArray<FString> Lineas;
    if (!FFileHelper::LoadFileToStringArray(Lineas, *JsonPath))
    {
        UE_LOG(LogTemp, Error, TEXT("RoadSurface: No se pudo cargar roads_unity.json"));
        return false;
    }

    FString JsonStr;
    for (const FString& L : Lineas) JsonStr += L;

    TSharedPtr<FJsonObject> Root;
    TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonStr);
    if (!FJsonSerializer::Deserialize(Reader, Root) || !Root.IsValid()) return false;

    const TArray<TSharedPtr<FJsonValue>>* Arr;
    if (!Root->TryGetArrayField(TEXT("roads"), Arr) && !Root->TryGetArrayField(TEXT(""), Arr)) return false;

    Superficies.Empty(Arr->Num());
    for (const auto& Val : *Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Obj->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() == 0) continue;
        const TSharedPtr<FJsonObject>& FirstPt = (*PointsArr)[0]->AsObject();
        if (!FirstPt) continue;

        FRoadSurfaceEntry Entry;
        Entry.Id = Obj->HasField(TEXT("id")) ? (int32)Obj->GetNumberField(TEXT("id")) : Superficies.Num();
        Entry.Nombre = Obj->HasField(TEXT("name")) ? Obj->GetStringField(TEXT("name")) : TEXT("");
        Entry.Calle = Entry.Nombre;
        Entry.X = FirstPt->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX;
        Entry.Z = FirstPt->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ;
        Entry.Ancho = Obj->HasField(TEXT("width")) ? Obj->GetNumberField(TEXT("width")) : 8.0f;
        Entry.Barrio = Obj->HasField(TEXT("barrio")) ? Obj->GetStringField(TEXT("barrio")) : TEXT("");
        Entry.Tipo = Obj->HasField(TEXT("type")) ? Obj->GetStringField(TEXT("type")) : TEXT("residential");

        Entry.Material = TEXT("asphalt");
        if (Entry.Tipo == TEXT("pedestrian") || Entry.Tipo == TEXT("footway"))
            Entry.Material = TEXT("cobblestone");
        else if (Entry.Tipo == TEXT("service"))
            Entry.Material = TEXT("asphalt_worn");
        else if (Entry.Tipo == TEXT("unclassified") || Entry.Tipo == TEXT("track"))
            Entry.Material = TEXT("gravel");
        else if (Entry.Barrio == TEXT("Herriko") || Entry.Barrio == TEXT("Harrobieta"))
            Entry.Material = TEXT("cobblestone");
        else if (Entry.Ancho < 5.0f)
            Entry.Material = TEXT("asphalt_worn");

        Superficies.Add(Entry);
        PorId.Add(Entry.Id, Superficies.Num() - 1);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("RoadSurface: %d tramos con superficie asignada"), Superficies.Num());
    return true;
}

bool UAlsasuaRoadSurfaceSystem::FirmeDe(int32 Id, FString& OutMaterial, FLinearColor& OutColor) const
{
    const int32* Idx = PorId.Find(Id);
    if (!Idx || !Superficies.IsValidIndex(*Idx)) return false;
    const FRoadSurfaceEntry& E = Superficies[*Idx];
    OutMaterial = E.Material;
    static const TMap<FString, FLinearColor> Colors = {
        {TEXT("asphalt"), FLinearColor(0.15f, 0.15f, 0.15f)},
        {TEXT("cobblestone"), FLinearColor(0.45f, 0.40f, 0.35f)},
        {TEXT("asphalt_worn"), FLinearColor(0.25f, 0.24f, 0.23f)},
        {TEXT("gravel"), FLinearColor(0.55f, 0.50f, 0.45f)},
    };
    if (const FLinearColor* C = Colors.Find(E.Material))
        OutColor = *C;
    else
        OutColor = FLinearColor(0.15f, 0.15f, 0.15f);
    return true;
}

//~ IAlsasuaPilarArranque (fase 26 del antiguo DirectorArranque)
int32 UAlsasuaRoadSurfaceSystem::EjecutarArranque()
{
	UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr;
	if (!W)
	{
		return -1;
	}
	int32 NumRoads = 0;
	for (TActorIterator<ACalleGenerada> It(W); It; ++It)
	{
		ACalleGenerada* C = *It;
		if (!C || !C->Malla) continue;

		FString Firme;
		FLinearColor Color = FLinearColor::Black;
		if (!FirmeDe(C->Id, Firme, Color)) continue;

		if (UMaterialInstanceDynamic* MID = C->Malla->CreateDynamicMaterialInstance(0))
		{
			// El nombre del parametro depende de que material haya cargado la
			// calle; si no lo tiene, el Set no hace nada y la calle se queda
			// con su color de tipo.
			MID->SetVectorParameterValue(FName(TEXT("Color")), Color);
			MID->SetVectorParameterValue(FName(TEXT("BaseColor")), Color);
		}
		++NumRoads;
	}
	return NumRoads;
}

FString UAlsasuaRoadSurfaceSystem::EtiquetaArranque() const
{
	return TEXT("firme aplicado a calles");
}

int32 UAlsasuaRoadSurfaceSystem::OrdenArranque() const
{
	return 260;
}
