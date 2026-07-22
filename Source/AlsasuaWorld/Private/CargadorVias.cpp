// CargadorVias.cpp
#include "CargadorVias.h"
#include "CalleGenerada.h"
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

// Material de agua creado por la utilidad de editor UCreadorMaterialAgua.
// Si no existe aún, los ríos quedan con su color de vértice (azulado).
static UMaterialInterface* CargarMaterialAgua()
{
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_AguaRio.M_AguaRio"));
}
static UMaterialInterface* CargarMaterialSuelo()
{
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
}

void UCargadorVias::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	if (bAutoCargar && !ArranqueMundo::HayDirector) Cargar();
}

void UCargadorVias::Encolar(const FString& RutaRel, FName Tag, float EpsilonCm,
                            float AnchoDefectoM, bool bAnchoPorTracks)
{
	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRel);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{ UE_LOG(LogTemp, Warning, TEXT("[Vias] omito %s (no existe)"), *Ruta); return; }

	TArray<TSharedPtr<FJsonValue>> Arr;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Arr)) return;

	for (const TSharedPtr<FJsonValue>& Val : Arr)
	{
		const TSharedPtr<FJsonObject> O = Val->AsObject();
		if (!O.IsValid()) continue;
		const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
		if (!O->TryGetArrayField(TEXT("pts"), Pts) || !Pts || Pts->Num() < 6) continue;  // >=2 puntos

		FTrabajoVia T;
		T.Tag = Tag; T.EpsilonCm = EpsilonCm;
		T.Tipo = O->HasField(TEXT("type")) ? O->GetStringField(TEXT("type")) : FString();

		// pts plano [x,y,z, x,y,z, ...] en mundo Unity ABSOLUTO (no se suma OX/OZ).
		for (int32 i = 0; i + 2 < Pts->Num(); i += 3)
		{
			const double ux = (*Pts)[i]->AsNumber();
			const double uz = (*Pts)[i + 2]->AsNumber();
			const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector(ux, 0.0, uz));
			T.PuntosMundo.Add(FVector2D(M.X, M.Y));
		}
		if (T.PuntosMundo.Num() < 2) continue;

		double AnchoM = AnchoDefectoM;
		if (bAnchoPorTracks)
			AnchoM = 1.6 * FMath::Max(1.0, O->HasField(TEXT("tracks")) ? O->GetNumberField(TEXT("tracks")) : 1.0) + 1.0;
		else if (O->HasField(TEXT("width")))
			AnchoM = O->GetNumberField(TEXT("width"));
		T.AnchoCm = (float)(AnchoM * 100.0);

		Trabajos.Add(MoveTemp(T));
	}
}

void UCargadorVias::PrepararCarga()
{
	if (bPreparado) return;
	bPreparado = true;
	Encolar(TEXT("Datos/footways_unity.json"),  TEXT("Acera"), 8.f,  3.f, false);
	Encolar(TEXT("Datos/railways_unity.json"),  TEXT("Via"),  14.f,  2.5f, true);
	Encolar(TEXT("Datos/waterways_unity.json"), TEXT("Agua"),-20.f,  6.f, false);   // río un poco hundido
	UE_LOG(LogTemp, Log, TEXT("[Vias] %d vías en cola"), Trabajos.Num());
}

bool UCargadorVias::PasoPresupuesto(double PresupuestoMs)
{
	if (!bPreparado) PrepararCarga();
	const double t0 = FPlatformTime::Seconds();
	while (Idx < Trabajos.Num())
	{
		const FTrabajoVia& T = Trabajos[Idx++];
		FActorSpawnParameters SP;
		SP.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;
		const FVector2D Centro = T.PuntosMundo[0];
		ACalleGenerada* C = GetWorld()->SpawnActor<ACalleGenerada>(
			ACalleGenerada::StaticClass(), FVector(Centro.X, Centro.Y, 0.f), FRotator::ZeroRotator, SP);
		if (C)
		{
			C->Tipo = T.Tipo;
			C->EpsilonCm = T.EpsilonCm;
			if (T.Tag != NAME_None) C->Tags.Add(T.Tag);
			// Color por tipo de vía.
			if      (T.Tag == TEXT("Acera")) C->ColorBase = FColor(120, 112, 100, 255); // adoquín claro
			else if (T.Tag == TEXT("Via"))   C->ColorBase = FColor(70, 62, 54, 255);    // balasto
			C->Construir(T.PuntosMundo, T.AnchoCm);

			if (C->Malla)
			{
				if (T.Tag == TEXT("Agua"))
				{
					static UMaterialInterface* MatAgua = CargarMaterialAgua();
					if (MatAgua) C->Malla->SetMaterial(0, MatAgua);
				}
				else
				{
					static UMaterialInterface* MatSuelo = CargarMaterialSuelo();
					if (MatSuelo) C->Malla->SetMaterial(0, MatSuelo);
				}
			}
			++Construidas;
		}
		if ((FPlatformTime::Seconds() - t0) * 1000.0 >= PresupuestoMs) break;
	}
	return Terminado();
}

int32 UCargadorVias::Cargar()
{
	if (bHecho) return 0;
	bHecho = true;
	PrepararCarga();
	while (!PasoPresupuesto(1000.0)) {}
	UE_LOG(LogTemp, Log, TEXT("[Vias] %d vías construidas (aceras+ferrocarril+ríos)"), Construidas);
	return Construidas;
}
