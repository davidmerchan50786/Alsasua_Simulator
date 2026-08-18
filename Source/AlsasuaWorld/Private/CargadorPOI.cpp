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

		// poi_data.json es ABSOLUTO en mundo Unity (x,z en metros) → mundo UE5.
		if (O->HasField(TEXT("x")) && O->HasField(TEXT("z")))
		{
			Data.PosicionMundo = UAlsasuaGeoData::UnityaUnreal(
				FVector(O->GetNumberField(TEXT("x")), 0.0, O->GetNumberField(TEXT("z"))));
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[POI] %s sin coordenadas x/z, omitido"), *Data.Nombre);
			continue;
		}

		TodosLosPOIs.Add(Data);
		ColocarPOI(Data);
		++Colocados;
	}

	UE_LOG(LogTemp, Log, TEXT("[POI] %d POIs cargados y colocados"), Colocados);
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
