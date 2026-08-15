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
#include "CargarMaterialComun.h"

// Material de agua creado por la utilidad de editor UCreadorMaterialAgua.
// Si no existe aún, los ríos quedan con su color de vértice (azulado).
static UMaterialInterface* CargarMaterialAgua()
{
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_AguaRio.M_AguaRio"));
}
static UMaterialInterface* CargarMaterialSueloVias()
{
	return CargarMaterialConFallbackSeguro(
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"),
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"),
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
}
static UMaterialInterface* CargarMaterialAcera()
{
	return CargarMaterialConFallback(
		TEXT("/Game/Road/Material/MI/M_Sidewalk_Master_Inst.M_Sidewalk_Master_Inst"),
		TEXT("/Game/Materiales/M_Terreno_Acera.M_Terreno_Acera"),
		TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
}

void UCargadorVias::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Las construye ADirectorArranque tras generar el terreno.
}

void UCargadorVias::Encolar(const FString& RutaRel, FName Tag, float EpsilonCm,
                            float AnchoDefectoM, bool bAnchoPorTracks,
                            const TCHAR* CampoArray)
{
	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRel);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{ UE_LOG(LogTemp, Warning, TEXT("[Vias] omito %s (no existe)"), *Ruta); return; }

	// Cuatro de los cinco datasets son un array en la raíz, pero railways_unity.json
	// es un objeto {"rails":[...], "stations":[...]} porque además de los trazados
	// lleva los dos apeaderos. Deserializar a TArray contra una raíz de objeto
	// devuelve false, así que la vía férrea entera se caía en silencio: 86
	// trazados y 38,7 km sin construir, y el director registrando "cargadas".
	// Por eso el tipo de raíz se resuelve aquí y no en el llamante.
	TArray<TSharedPtr<FJsonValue>> Arr;
	{
		const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
		if (!FJsonSerializer::Deserialize(R, Arr))
		{
			TSharedPtr<FJsonObject> Doc;
			const TSharedRef<TJsonReader<>> R2 = TJsonReaderFactory<>::Create(Texto);
			const TArray<TSharedPtr<FJsonValue>>* Campo = nullptr;
			if (!CampoArray || !FJsonSerializer::Deserialize(R2, Doc) || !Doc.IsValid()
				|| !Doc->TryGetArrayField(CampoArray, Campo) || !Campo)
			{
				UE_LOG(LogTemp, Warning, TEXT("[Vias] %s no tiene la forma esperada"), *Ruta);
				return;
			}
			Arr = *Campo;
		}
	}

	int32 Encoladas = 0;
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
		{
			// El andén no es plataforma de balasto: es solado, y va con el ancho
			// y el acabado de una acera. OSM lo mete en la misma capa que la vía.
			if (T.Tipo == TEXT("platform"))
			{
				T.Tag = TEXT("Acera");
				AnchoM = 4.0;
			}
			else
			{
				// OSM etiqueta cada vía como un way independiente, así que aquí
				// tracks siempre vale 1 y el ancho es el de una plataforma de vía
				// única: traviesa de 2,6 m más los hombros de balasto, ~4,4 m. Con
				// la fórmula anterior (1,6·tracks+1) salían 2,6 m, la traviesa
				// pelada. Cabe de sobra en la playa de vías, que las separa 4,5 m.
				AnchoM = 1.6 * FMath::Max(1.0, O->HasField(TEXT("tracks")) ? O->GetNumberField(TEXT("tracks")) : 1.0) + 2.8;
			}
		}
		else if (O->HasField(TEXT("width")))
			AnchoM = O->GetNumberField(TEXT("width"));
		T.AnchoCm = (float)(AnchoM * 100.0);

		Trabajos.Add(MoveTemp(T));
		++Encoladas;
	}

	UE_LOG(LogTemp, Log, TEXT("[Vias] %s: %d trazados"), *RutaRel, Encoladas);
}

void UCargadorVias::PrepararCarga()
{
	if (bPreparado) return;
	bPreparado = true;
	Encolar(TEXT("Datos/footways_unity.json"),  TEXT("Acera"),  8.f,  3.f, false);
	Encolar(TEXT("Datos/railways_unity.json"),  TEXT("Via"),   14.f,  2.5f, true, TEXT("rails"));
	Encolar(TEXT("Datos/waterways_unity.json"), TEXT("Agua"), -20.f,  6.f, false);   // río un poco hundido
	Encolar(TEXT("Datos/caminos_unity.json"),   TEXT("Camino"), 6.f,  3.f, false);   // pistas/senderos de monte
	Encolar(TEXT("Datos/tunnels_unity.json"),   TEXT("Tunel"),  8.f,  4.f, false);   // túneles (placeholder hasta ATunelAlsasua)
	UE_LOG(LogTemp, Log, TEXT("[Vias] %d vías en cola"), Trabajos.Num());
}

bool UCargadorVias::PasoPresupuesto(double PresupuestoMs)
{
	if (!bPreparado) PrepararCarga();
	const double t0 = FPlatformTime::Seconds();
	while (Idx < Trabajos.Num())
	{
		const FTrabajoVia& T = Trabajos[Idx++];

		// Túneles: datos cargados para uso futuro de ATunelAlsasua (paso 3).
		// Por ahora se omite la malla visible para evitar artefactos en superficie.
		if (T.Tag == TEXT("Tunel")) { ++Construidas; continue; }

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
			if      (T.Tag == TEXT("Acera"))  C->ColorBase = FColor(120, 112, 100, 255); // adoquín claro
			else if (T.Tag == TEXT("Via"))    C->ColorBase = FColor( 70,  62,  54, 255); // balasto
			else if (T.Tag == TEXT("Camino")) C->ColorBase = FColor(130, 105,  70, 255); // tierra/grava marrón
			C->Construir(T.PuntosMundo, T.AnchoCm);

			if (C->Malla)
			{
				if (T.Tag == TEXT("Agua"))
				{
					static UMaterialInterface* MatAgua = CargarMaterialAgua();
					if (MatAgua) C->Malla->SetMaterial(0, MatAgua);
				}
				else if (T.Tag == TEXT("Acera"))
				{
					static UMaterialInterface* MatAcera = CargarMaterialAcera();
					if (MatAcera) C->Malla->SetMaterial(0, MatAcera);
				}
				else
				{
					static UMaterialInterface* MatSuelo = CargarMaterialSueloVias();
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
	int32 IterGuard = 0;
	const int32 MaxIter = 10000;
	while (!PasoPresupuesto(1000.0) && ++IterGuard < MaxIter) {}
	if (IterGuard >= MaxIter) UE_LOG(LogTemp, Warning, TEXT("[Vias] Iteration guard reached (%d)"), MaxIter);
	UE_LOG(LogTemp, Log, TEXT("[Vias] %d vías construidas (aceras+ferrocarril+ríos+caminos+túneles)"), Construidas);
	return Construidas;
}
