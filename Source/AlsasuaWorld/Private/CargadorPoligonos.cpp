// CargadorPoligonos.cpp
#include "CargadorPoligonos.h"
#include "PoligonoSuelo.h"
#include "ArranqueMundo.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "HAL/PlatformTime.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"

static UMaterialInterface* CargarMaterialSueloPoligonos()
{
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
}

void UCargadorPoligonos::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (bAutoCargar && !ArranqueMundo::HayDirector) Cargar();
}

void UCargadorPoligonos::Encolar(const FString& RutaRel, FColor ColorDefecto, float EpsilonCm)
{
	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRel);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{ UE_LOG(LogTemp, Warning, TEXT("[Suelos] omito %s"), *Ruta); return; }

	TArray<TSharedPtr<FJsonValue>> Arr;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Arr)) return;

	for (const TSharedPtr<FJsonValue>& Val : Arr)
	{
		const TSharedPtr<FJsonObject> O = Val->AsObject();
		if (!O.IsValid()) continue;
		const TArray<TSharedPtr<FJsonValue>>* Poly = nullptr;
		if (!O->TryGetArrayField(TEXT("poly"), Poly) || !Poly || Poly->Num() < 6) continue;  // >=3 pares

		FTrabajoSuelo T;
		T.EpsilonCm = EpsilonCm;
		T.Tipo  = O->HasField(TEXT("type")) ? O->GetStringField(TEXT("type")) : FString();
		T.Color = ColorDefecto;
		if (O->HasField(TEXT("color")))
		{
			FString Hex = O->GetStringField(TEXT("color"));
			if (!Hex.IsEmpty()) T.Color = FColor::FromHex(Hex);
		}

		// poly plano [x,z, x,z, ...] en mundo Unity ABSOLUTO.
		for (int32 i = 0; i + 1 < Poly->Num(); i += 2)
		{
			const double ux = (*Poly)[i]->AsNumber();
			const double uz = (*Poly)[i + 1]->AsNumber();
			const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector(ux, 0.0, uz));
			T.Anillo.Add(FVector2D(M.X, M.Y));
		}
		if (T.Anillo.Num() >= 3) Trabajos.Add(MoveTemp(T));
	}
}

void UCargadorPoligonos::PrepararCarga()
{
	if (bPreparado) return;
	bPreparado = true;
	Encolar(TEXT("Datos/greenspaces_unity.json"), FColor(58, 122, 53), 5.f);   // verde parque
	Encolar(TEXT("Datos/plazas_unity.json"),      FColor(150, 145, 135), 7.f); // adoquín claro
	UE_LOG(LogTemp, Log, TEXT("[Suelos] %d polígonos en cola"), Trabajos.Num());
}

bool UCargadorPoligonos::PasoPresupuesto(double PresupuestoMs)
{
	if (!bPreparado) PrepararCarga();
	const double t0 = FPlatformTime::Seconds();
	while (Idx < Trabajos.Num())
	{
		const FTrabajoSuelo& T = Trabajos[Idx++];
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector2D Centro = T.Anillo[0];
		APoligonoSuelo* S = GetWorld()->SpawnActor<APoligonoSuelo>(
			APoligonoSuelo::StaticClass(), FVector(Centro.X, Centro.Y, 0.f), FRotator::ZeroRotator, SP);
		if (S)
		{
			S->Tipo = T.Tipo;
			S->Construir(T.Anillo, T.Color, T.EpsilonCm);
			if (S->Malla) { static UMaterialInterface* Mat = CargarMaterialSueloPoligonos(); if (Mat) S->Malla->SetMaterial(0, Mat); }
			++Construidos;
		}
		if ((FPlatformTime::Seconds() - t0) * 1000.0 >= PresupuestoMs) break;
	}
	return Terminado();
}

int32 UCargadorPoligonos::Cargar()
{
	if (bHecho) return 0;
	bHecho = true;
	PrepararCarga();
	int32 IterGuard = 0;
	const int32 MaxIter = 10000;
	while (!PasoPresupuesto(1000.0) && ++IterGuard < MaxIter) {}
	if (IterGuard >= MaxIter) UE_LOG(LogTemp, Warning, TEXT("[Suelos] Iteration guard reached (%d)"), MaxIter);
	UE_LOG(LogTemp, Log, TEXT("[Suelos] %d polígonos (plazas+zonas verdes)"), Construidos);
	return Construidos;
}
