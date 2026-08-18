// CargadorPOI.cpp
#include "CargadorPOI.h"
#include "Components/TextRenderComponent.h"
#include "Components/BillboardComponent.h"
#include "Engine/StaticMeshActor.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "GeoDataAlsasua.h"
#include "Engine/Engine.h"

FString UCargadorPOI::GetDebugSummary() const
{
	return FString::Printf(TEXT("Loaded=%d | Total=%d"), TodosLosPOIs.Num(), TodosLosPOIs.Num());
}

void UCargadorPOI::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (bAutoCargar)
	{
		GetWorld()->GetTimerManager().SetTimerForNextTick([this]() { Cargar(); });
	}
}

int32 UCargadorPOI::Cargar()
{
	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRelativa);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{
		UE_LOG(LogTemp, Warning, TEXT("[POI] No se pudo cargar %s"), *Ruta);
		return 0;
	}

	// Parsear como objeto raíz {"pois": [...]}.
	TSharedPtr<FJsonValue> RootVal;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, RootVal)) return 0;

	TSharedPtr<FJsonObject> RootObj = RootVal->AsObject();
	const TArray<TSharedPtr<FJsonValue>>* PoisArr = nullptr;
	if (!RootObj.IsValid() || !RootObj->TryGetArrayField(TEXT("pois"), PoisArr)) return 0;

	const TArray<TSharedPtr<FJsonValue>>& Items = *PoisArr;

	int32 Colocados = 0;
	int32 PorLatLon = 0, PorXZ = 0;
	for (const auto& Item : Items)
	{
		const auto& O = Item->AsObject();
		if (!O.IsValid()) continue;

		FPOIData Data;
		Data.Id = O->GetStringField(TEXT("id"));
		Data.Nombre = O->GetStringField(TEXT("nombre"));
		Data.Tipo = O->GetStringField(TEXT("tipo"));
		Data.Subtipo = O->GetStringField(TEXT("subtipo"));
		Data.Calle = O->HasField(TEXT("calle")) ? O->GetStringField(TEXT("calle")) : FString();
		Data.Descripcion = O->HasField(TEXT("descripcion")) ? O->GetStringField(TEXT("descripcion")) : FString();
		Data.bInteractuable = O->HasField(TEXT("interactuable")) && O->GetBoolField(TEXT("interactuable"));
		Data.bExtorsionable = O->HasField(TEXT("extorsionable")) && O->GetBoolField(TEXT("extorsionable"));
		Data.Faccion = O->HasField(TEXT("faccion")) ? O->GetStringField(TEXT("faccion")) : FString();
		Data.Dialogo = O->HasField(TEXT("dialogo")) ? O->GetStringField(TEXT("dialogo")) : FString();

		// Cuando el POI trae lat/lon, mandan ellas.
		//
		// Los x/z de poi_data.json no cuadran con el pueblo: comparando los 30
		// que además traen lat/lon, las distancias entre POIs salen 10 veces más
		// pequeñas por x/z que por coordenada geográfica, y la z va al revés —el
		// factor es -10, no +10—. Colocados por x/z, esos 30 quedan a 118 m de
		// mediana del edificio más cercano y ninguno cae sobre uno; por lat/lon
		// la mediana baja a 14 m y once caen justo encima del footprint. Le pasa
		// lo mismo a landmarks_real.json, que se generó igual.
		//
		// Los 47 sin lat/lon se quedan con su x/z, que es lo único que hay: ésos
		// sí alcanzan edificios (mediana 78 m), así que vienen de otra pasada.
		// Tools/VerificarDatasets.py contrasta los dos marcos y canta la
		// discrepancia.
		double Lat = 0.0, Lon = 0.0;
		if (O->TryGetNumberField(TEXT("lat"), Lat) && O->TryGetNumberField(TEXT("lon"), Lon))
		{
			Data.PosicionMundo = UAlsasuaGeoData::LatLonToUE5(Lat, Lon);
			++PorLatLon;
		}
		else if (O->HasField(TEXT("x")) && O->HasField(TEXT("z")))
		{
			Data.PosicionMundo = UAlsasuaGeoData::AbsLocalToUE5(
				FVector(O->GetNumberField(TEXT("x")), 0.0, O->GetNumberField(TEXT("z"))));
			++PorXZ;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[POI] %s sin coordenadas, omitido"), *Data.Nombre);
			continue;
		}

		// Los dos conversores dejan la Z a cero, que es cota cero del mundo:
		// 531 m por debajo del pueblo. Hay que apoyarla en el terreno.
		Data.PosicionMundo.Z = UAlsasuaGeoData::AlturaSueloUE5(
			GetWorld(), Data.PosicionMundo.X, Data.PosicionMundo.Y);

		TodosLosPOIs.Add(Data);
		ColocarPOI(Data);
		++Colocados;
	}

	UE_LOG(LogTemp, Log, TEXT("[POI] %d POIs colocados (%d por lat/lon, %d por x/z)"),
		Colocados, PorLatLon, PorXZ);
	UE_LOG(LogTemp, Log, TEXT("[POI] %s"), *GetDebugSummary());
	return Colocados;
}

void UCargadorPOI::ColocarPOI(const FPOIData& Data)
{
	UWorld* World = GetWorld();
	if (!World) return;

	// Crear actor con billboard + texto 3D para debug.
	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(),
		Data.PosicionMundo, FRotator::ZeroRotator);
	if (!Actor) return;

	// Billboard icon (solo visible en editor).
	UBillboardComponent* Billboard = NewObject<UBillboardComponent>(Actor);
	Billboard->RegisterComponent();
	Billboard->SetupAttachment(Actor->GetRootComponent());
	Billboard->bHiddenInGame = true;

	// Texto 3D con el nombre del POI.
	UTextRenderComponent* Texto = NewObject<UTextRenderComponent>(Actor);
	Texto->RegisterComponent();
	Texto->SetupAttachment(Actor->GetRootComponent());
	Texto->SetText(FText::FromString(Data.Nombre));
	Texto->SetTextRenderColor(FColor::Yellow);
	Texto->SetHorizontalAlignment(EHTA_Center);
	Texto->SetWorldSize(200.f);
	Texto->SetRelativeLocation(FVector(0, 0, 300.f));
	Texto->SetVisibility(true);

	// Tags de gameplay.
	Actor->Tags.Add(FName(*Data.Tipo));
	Actor->Tags.Add(FName("POI"));
	if (Data.bExtorsionable) Actor->Tags.Add(FName("Extorsionable"));
	if (Data.bInteractuable) Actor->Tags.Add(FName("Interactuable"));

#if WITH_EDITOR
	Actor->SetActorLabel(Data.Nombre);
#endif
}

TArray<FPOIData> UCargadorPOI::GetPOIsByTipo(const FString& Tipo) const
{
	TArray<FPOIData> Result;
	for (const FPOIData& P : TodosLosPOIs)
	{
		if (P.Tipo == Tipo) Result.Add(P);
	}
	return Result;
}

FPOIData UCargadorPOI::GetPOIById(const FString& Id) const
{
	for (const FPOIData& P : TodosLosPOIs)
	{
		if (P.Id == Id) return P;
	}
	return FPOIData();
}
