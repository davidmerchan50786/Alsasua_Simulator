#include "World/AlsasuaRoadSurfaceSystem.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "CargarJsonComun.h"

void UAlsasuaRoadSurfaceSystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CargarSuperficies();
}

void UAlsasuaRoadSurfaceSystem::Deinitialize()
{
    Superficies.Empty();
    PorId.Empty();
    bCargado = false;
    Super::Deinitialize();
}

bool UAlsasuaRoadSurfaceSystem::CargarSuperficies()
{
    // roads_unity.json es un array en la raíz, no un objeto con campo "roads".
    // Esto leía la raíz como FJsonObject, la deserialización devolvía false y el
    // sistema entero salía por aquí sin decir nada: ni una superficie de calle en
    // todo el pueblo. JsonDatos::CargarArray se traga cualquiera de las dos formas.
    TArray<TSharedPtr<FJsonValue>> Arr;
    if (!JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Arr, { TEXT("roads") }))
    {
        UE_LOG(LogTemp, Error, TEXT("RoadSurface: sin vías en roads_unity.json"));
        return false;
    }

    Superficies.Empty(Arr.Num());
    PorId.Empty(Arr.Num());
    for (const auto& Val : Arr)
    {
        const TSharedPtr<FJsonObject>& Obj = Val->AsObject();
        if (!Obj) continue;

        const TArray<TSharedPtr<FJsonValue>>* PointsArr;
        if (!Obj->TryGetArrayField(TEXT("points"), PointsArr) || !PointsArr || PointsArr->Num() == 0) continue;
        const TSharedPtr<FJsonObject>& FirstPt = (*PointsArr)[0]->AsObject();
        if (!FirstPt) continue;

        FRoadSurfaceEntry Entry;
        Entry.Id = Obj->HasField(TEXT("id")) ? (int32)Obj->GetIntegerField(TEXT("id")) : -1;
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

        if (Entry.Id >= 0) PorId.Add(Entry.Id, Superficies.Num());
        Superficies.Add(Entry);
    }

    bCargado = true;
    UE_LOG(LogTemp, Log, TEXT("RoadSurface: %d tramos con superficie asignada"), Superficies.Num());
    return true;
}

bool UAlsasuaRoadSurfaceSystem::FirmeDe(int32 Id, FString& OutMaterial, FLinearColor& OutColor) const
{
    // Lo pide ADirectorArranque (fase 26) para teñir las ACalleGenerada que
    // UCargadorCalles ya dejó drapeadas en la fase 4. Aquí no se construye nada:
    // este sistema clasifica el firme, no pone calzada.
    static const TMap<FString, FLinearColor> Colores = {
        { TEXT("asphalt"),      FLinearColor(0.15f, 0.15f, 0.15f) },
        { TEXT("cobblestone"),  FLinearColor(0.45f, 0.40f, 0.35f) },
        { TEXT("asphalt_worn"), FLinearColor(0.25f, 0.24f, 0.23f) },
        { TEXT("gravel"),       FLinearColor(0.55f, 0.50f, 0.45f) },
    };

    const int32* Idx = PorId.Find(Id);
    if (!Idx || !Superficies.IsValidIndex(*Idx)) return false;

    const FRoadSurfaceEntry& E = Superficies[*Idx];
    OutMaterial = E.Material;
    const FLinearColor* C = Colores.Find(E.Material);
    OutColor = C ? *C : Colores[TEXT("asphalt")];
    return true;
}
