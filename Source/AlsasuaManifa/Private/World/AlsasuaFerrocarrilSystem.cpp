#include "World/AlsasuaFerrocarrilSystem.h"
#include "World/AlsasuaMallaFab.h"
#include "GeoDataAlsasua.h"
#include "AjusteMallaComun.h"
#include "Engine/World.h"
#include "Engine/StaticMesh.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/** Medidas reales del vehículo (m): largo, ancho, alto. */
	constexpr double LargoLocomotoraM = 18.0, AnchoLocomotoraM = 3.0, AltoLocomotoraM = 4.3;
	constexpr double LargoVagonM      = 12.2, AnchoVagonM      = 2.44, AltoVagonM     = 2.6;
	/** Hueco entre topes de dos vehículos consecutivos (m). */
	constexpr double SeparacionM      = 1.8;
	/** Del suelo drapeado al plano de rodadura: balasto + traviesa + carril. */
	constexpr float  AlturaCarrilCm   = 32.f;

	/** Malla resuelta y cómo hay que ponerla para que mida lo que debe. */
	struct FVehiculo
	{
		UStaticMesh* Malla = nullptr;
		AjusteMalla::FColocacion Col;
	};

	struct FTrazado
	{
		TArray<FVector2D> Pts;      // cm de mundo
		TArray<double>    Acum;     // longitud acumulada hasta cada punto (cm)
		double            Largo = 0.0;
		bool              bElectrificada = false;
		double            DistEstacionCm = 0.0;
	};

	/** Punto y rumbo a una distancia dada desde el inicio del trazado. */
	bool PuntoEn(const FTrazado& T, double SCm, FVector2D& OutXY, double& OutYawDeg)
	{
		if (T.Pts.Num() < 2 || SCm < 0.0 || SCm > T.Largo) return false;
		int32 i = 1;
		while (i < T.Acum.Num() - 1 && T.Acum[i] < SCm) ++i;

		const double Tramo = T.Acum[i] - T.Acum[i - 1];
		const double f = Tramo > 1e-3 ? (SCm - T.Acum[i - 1]) / Tramo : 0.0;
		const FVector2D D = T.Pts[i] - T.Pts[i - 1];
		OutXY = T.Pts[i - 1] + D * f;
		OutYawDeg = FMath::RadiansToDegrees(FMath::Atan2(D.Y, D.X));
		return true;
	}

	/**
	 * Resuelve la malla de un papel y calcula cómo colocarla para que mida lo
	 * que mide el vehículo de verdad.
	 *
	 * Escala uniforme si hay malla modelada — deformar una locomotora para
	 * cuadrarle el ancho la estropearía — y encaje de las tres medidas si sólo
	 * hay forma básica del motor: un vagón con la silueta correcta se lee como
	 * vagón, y un cubo de 18 m de lado, que es lo que saldría de escalar
	 * uniforme un cubo, no se lee como nada.
	 */
	FVehiculo Preparar(const FString& Tipo, const FVector& MedidasM)
	{
		FVehiculo V;
		V.Malla = AlsasuaMallaFab::Resolver(Tipo, TEXT("/Engine/BasicShapes/Cube.Cube"));
		V.Col = AjusteMalla::Calcular(V.Malla, MedidasM, AlsasuaMallaFab::VieneDeFab(Tipo));
		return V;
	}
}

int32 UAlsasuaFerrocarrilSystem::ColocarMaterialRodante()
{
	UWorld* World = GetWorld();
	if (!World || bHecho) return 0;
	bHecho = true;

	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(),
		TEXT("Datos/railways_unity.json"));
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{
		UE_LOG(LogTemp, Warning, TEXT("Ferrocarril: sin %s; no hay material rodante."), *Ruta);
		return 0;
	}

	TSharedPtr<FJsonObject> Doc;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Doc) || !Doc.IsValid()) return 0;

	// --- Estación de referencia -------------------------------------------
	// Se aparca junto a la estación de Altsasu, no junto al apeadero de
	// Altsasu-Herria: el apeadero es un andén sin apartadero donde no cabe una
	// composición de mercancías parada.
	const TArray<TSharedPtr<FJsonValue>>* Estaciones = nullptr;
	FVector2D EstacionCm = FVector2D::ZeroVector;
	bool bHayEstacion = false;
	if (Doc->TryGetArrayField(TEXT("stations"), Estaciones) && Estaciones)
	{
		for (const TSharedPtr<FJsonValue>& V : *Estaciones)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			if (!O.IsValid() || !O->HasField(TEXT("type"))) continue;
			if (O->GetStringField(TEXT("type")) != TEXT("station")) continue;
			const FVector P = UAlsasuaGeoData::UnityaUnreal(
				FVector(O->GetNumberField(TEXT("x")), 0.0, O->GetNumberField(TEXT("z"))));
			EstacionCm = FVector2D(P.X, P.Y);
			bHayEstacion = true;
			break;
		}
	}
	if (!bHayEstacion)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ferrocarril: railways_unity.json sin estación."));
		return 0;
	}

	// --- Trazados candidatos ----------------------------------------------
	const double LargoComposicionCm =
		(LargoLocomotoraM + VagonesPorComposicion * (LargoVagonM + SeparacionM) + SeparacionM) * 100.0;
	const double RadioCm = RadioEstacionM * 100.0;

	const TArray<TSharedPtr<FJsonValue>>* Rails = nullptr;
	if (!Doc->TryGetArrayField(TEXT("rails"), Rails) || !Rails) return 0;

	TArray<FTrazado> Candidatos;
	for (const TSharedPtr<FJsonValue>& V : *Rails)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O.IsValid()) continue;
		// El andén no es vía: por ahí no circula ni se aparca nada.
		if (O->HasField(TEXT("type")) && O->GetStringField(TEXT("type")) != TEXT("rail")) continue;

		const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
		if (!O->TryGetArrayField(TEXT("pts"), Pts) || !Pts || Pts->Num() < 6) continue;

		FTrazado T;
		T.bElectrificada = O->HasField(TEXT("electrified"))
			&& O->GetStringField(TEXT("electrified")) != TEXT("no");
		for (int32 i = 0; i + 2 < Pts->Num(); i += 3)
		{
			const FVector M = UAlsasuaGeoData::UnityaUnreal(
				FVector((*Pts)[i]->AsNumber(), 0.0, (*Pts)[i + 2]->AsNumber()));
			T.Pts.Add(FVector2D(M.X, M.Y));
		}
		if (T.Pts.Num() < 2) continue;

		T.Acum.Add(0.0);
		double DMin = TNumericLimits<double>::Max();
		for (int32 i = 0; i < T.Pts.Num(); ++i)
		{
			if (i > 0) T.Acum.Add(T.Acum[i - 1] + FVector2D::Distance(T.Pts[i - 1], T.Pts[i]));
			DMin = FMath::Min(DMin, (double)FVector2D::Distance(T.Pts[i], EstacionCm));
		}
		T.Largo = T.Acum.Last();
		T.DistEstacionCm = DMin;

		if (DMin > RadioCm) continue;
		if (T.Largo < LargoComposicionCm * 1.2) continue;   // sin sitio para parar

		Candidatos.Add(MoveTemp(T));
	}

	if (Candidatos.Num() == 0)
	{
		UE_LOG(LogTemp, Log, TEXT("Ferrocarril: ninguna vía de %.0f m junto a la estación."),
			LargoComposicionCm / 100.0);
		return 0;
	}

	// Primero los apartaderos sin catenaria y, a igualdad, los más cercanos a la
	// estación: dejar una rastra de mercancías parada en la vía general
	// electrificada sería dejarla cortando la línea Madril-Hendaia.
	Candidatos.Sort([](const FTrazado& A, const FTrazado& B)
	{
		if (A.bElectrificada != B.bElectrificada) return !A.bElectrificada;
		return A.DistEstacionCm < B.DistEstacionCm;
	});

	// --- Mallas -------------------------------------------------------------
	const FVehiculo Loco  = Preparar(TEXT("locomotora"),
		FVector(LargoLocomotoraM, AnchoLocomotoraM, AltoLocomotoraM));
	const FVehiculo Vagon = Preparar(TEXT("vagon_contenedor"),
		FVector(LargoVagonM, AnchoVagonM, AltoVagonM));
	if (!Loco.Col.bValido && !Vagon.Col.bValido)
	{
		UE_LOG(LogTemp, Warning, TEXT("Ferrocarril: sin malla utilizable; vías vacías."));
		return 0;
	}

	// --- Composiciones ------------------------------------------------------
	int32 Puestos = 0;
	const int32 NumComp = FMath::Min(MaxComposiciones, Candidatos.Num());
	for (int32 c = 0; c < NumComp; ++c)
	{
		const FTrazado& T = Candidatos[c];
		// S es el centro de cada vehículo, así que la composición arranca medio
		// largo de locomotora dentro. Centrada en la vía para que no asome medio
		// tren por la aguja del final.
		double S = (T.Largo - LargoComposicionCm) * 0.5 + LargoLocomotoraM * 0.5 * 100.0;

		for (int32 v = 0; v <= VagonesPorComposicion; ++v)
		{
			const bool bLoco = (v == 0);
			const FVehiculo& Veh = bLoco ? Loco : Vagon;

			FVector2D XY; double Yaw = 0.0;
			if (!PuntoEn(T, S, XY, Yaw)) break;

			// Si falta la malla de este papel se deja el hueco y se sigue: media
			// composición bien puesta vale más que una fila de cubos, y el hueco
			// se rellena solo en cuanto el asset esté importado.
			if (Veh.Col.bValido)
			{
				const float SueloZ = UAlsasuaGeoData::AlturaSueloUE5(World, XY.X, XY.Y);
				const FVector Loc(XY.X, XY.Y, SueloZ + AlturaCarrilCm + Veh.Col.SubirCm);
				const FRotator Rot(0.f, (float)Yaw + Veh.Col.YawExtra, 0.f);

				if (AStaticMeshActor* A = World->SpawnActor<AStaticMeshActor>(
						AStaticMeshActor::StaticClass(), Loc, Rot))
				{
					A->SetMobility(EComponentMobility::Movable);
					A->SetActorScale3D(Veh.Col.Escala);
					A->GetStaticMeshComponent()->SetStaticMesh(Veh.Malla);
#if WITH_EDITOR
					A->SetActorLabel(*FString::Printf(TEXT("Tren_%d_%s%d"), c,
						bLoco ? TEXT("Loco") : TEXT("Vagon"), v));
#endif
					++Puestos;
				}
			}

			S += ((bLoco ? LargoLocomotoraM : LargoVagonM) * 0.5
			      + LargoVagonM * 0.5 + SeparacionM) * 100.0;
		}
	}

	UE_LOG(LogTemp, Log,
		TEXT("Ferrocarril: %d vehículos en %d composiciones (%d vías candidatas)."),
		Puestos, NumComp, Candidatos.Num());
	return Puestos;
}
