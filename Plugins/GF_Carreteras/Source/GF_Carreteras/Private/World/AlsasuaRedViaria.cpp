// AlsasuaRedViaria.cpp
#include "World/AlsasuaRedViaria.h"
#include "CargarJsonComun.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"
#include "Algo/Reverse.h"
#include "AlsasuaServiceRegistry.h"

namespace
{
	/**
	 * Clave de nodo. Los cruces de OSM comparten coordenada EXACTA, así que
	 * redondear a milímetro basta y no hace falta encaje por proximidad —que
	 * además uniría calles que sólo pasan cerca, como un paso elevado sobre una
	 * calle—. Con este criterio sólo 3 de las 402 vías conducibles quedan sin
	 * conectar, medido con Tools/VerificarRedViaria.py.
	 */
	FIntPoint ClaveNodo(double X, double Z)
	{
		return FIntPoint(FMath::RoundToInt(X * 1000.0), FMath::RoundToInt(Z * 1000.0));
	}
}

bool UAlsasuaRedViaria::EsConducible(const FString& Tipo)
{
	// Tiene que decir lo mismo que CONDUCIBLE de Tools/VerificarRedViaria.py.
	static const TSet<FString> Conducibles = {
		TEXT("motorway"), TEXT("motorway_link"), TEXT("trunk"), TEXT("trunk_link"),
		TEXT("primary"), TEXT("secondary"), TEXT("tertiary"), TEXT("residential"),
		TEXT("service"), TEXT("unclassified"), TEXT("living_street")
	};
	return Conducibles.Contains(Tipo);
}

bool UAlsasuaRedViaria::EsTransitableAPie(const FString& Tipo)
{
	// Por exclusión, que es como funciona a pie: se puede andar por casi todo
	// menos por la autovía y sus enlaces. Con lista blanca habría que acordarse
	// de meter cada tipo nuevo del dataset, y olvidarse deja el pueblo sin
	// peatones en esa calle sin decir nada.
	static const TSet<FString> Prohibidas = {
		TEXT("motorway"), TEXT("motorway_link"), TEXT("trunk"), TEXT("trunk_link")
	};
	return !Prohibidas.Contains(Tipo);
}

int32 UAlsasuaRedViaria::Construir()
{
	if (Tramos.Num() > 0) return Tramos.Num();

	UWorld* W = GetWorld();
	if (!W) return 0;

	TArray<TSharedPtr<FJsonValue>> Vias;
	if (!JsonDatos::CargarArray(TEXT("Datos/roads_unity.json"), Vias))
	{
		UE_LOG(LogTemp, Warning, TEXT("RedViaria: sin roads_unity.json; no hay red."));
		return 0;
	}

	TMap<FIntPoint, int32> Indice;
	int32 Descartadas = 0, Unicas = 0;
	double LargoTotal = 0.0;

	auto NodoDe = [&](double X, double Z) -> int32
	{
		const FIntPoint K = ClaveNodo(X, Z);
		if (const int32* Ya = Indice.Find(K)) return *Ya;
		// La cota se apoya en el terreno una sola vez por nodo. Son ~2300
		// muestreos, y desde que ATerrenoGenerado registra su heightmap eso es
		// una interpolación bilineal, no un trazo de física.
		const FVector P = UAlsasuaGeoData::RelLocalASueloUE5(W, FVector(X, 0.0, Z));
		const int32 Nuevo = Nodos.Add(P);
		Indice.Add(K, Nuevo);
		return Nuevo;
	};

	for (const TSharedPtr<FJsonValue>& V : Vias)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O.IsValid()) continue;

		FString Tipo;
		O->TryGetStringField(TEXT("type"), Tipo);
		if (!EsConducible(Tipo)) { ++Descartadas; continue; }

		const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
		// Dos puntos bastan para un tramo. El sistema de tráfico exigía cuatro y
		// eso tiraba 192 de las 489 vías, el 39%: la mediana del dataset es
		// justo 4, así que el corte caía en mitad de la distribución.
		if (!O->TryGetArrayField(TEXT("points"), Pts) || !Pts || Pts->Num() < 2) continue;

		const bool bUnica = O->HasField(TEXT("oneway")) && O->GetBoolField(TEXT("oneway"));
		if (bUnica) ++Unicas;

		FTramoViario Base;
		Base.Tipo = Tipo;
		Base.IdVia = O->HasField(TEXT("id")) ? (int32)O->GetNumberField(TEXT("id")) : 0;
		Base.AnchoCm = (O->HasField(TEXT("width")) ? (float)O->GetNumberField(TEXT("width")) : 5.f) * 100.f;
		O->TryGetStringField(TEXT("name"), Base.Nombre);

		for (int32 i = 0; i + 1 < Pts->Num(); ++i)
		{
			const TSharedPtr<FJsonObject> A = (*Pts)[i]->AsObject();
			const TSharedPtr<FJsonObject> B = (*Pts)[i + 1]->AsObject();
			if (!A.IsValid() || !B.IsValid()) continue;

			const int32 NA = NodoDe(A->GetNumberField(TEXT("x")), A->GetNumberField(TEXT("z")));
			const int32 NB = NodoDe(B->GetNumberField(TEXT("x")), B->GetNumberField(TEXT("z")));
			if (NA == NB) continue;   // punto repetido en el dato

			const float Largo = FVector::Dist2D(Nodos[NA], Nodos[NB]);
			LargoTotal += Largo;

			FTramoViario T = Base;
			T.NodoA = NA; T.NodoB = NB; T.LargoCm = Largo;
			Tramos.Add(T);

			// El sentido contrario sólo si la vía no es de sentido único.
			if (!bUnica)
			{
				FTramoViario R = T;
				R.NodoA = NB; R.NodoB = NA;
				Tramos.Add(R);
			}
		}
	}

	// Índice de salidas por nodo, que es lo que permite girar en un cruce.
	Salidas.SetNum(Nodos.Num());
	for (int32 i = 0; i < Tramos.Num(); ++i)
	{
		Salidas[Tramos[i].NodoA].Add(i);
	}

	int32 Cruces = 0, Fondos = 0;
	for (const TArray<int32>& S : Salidas)
	{
		if (S.Num() > 2) ++Cruces;
		else if (S.Num() <= 1) ++Fondos;
	}

	UE_LOG(LogTemp, Log,
		TEXT("RedViaria: %d nodos, %d tramos dirigidos, %d cruces, %d fondos de saco, "
		     "%.1f km. %d vías descartadas por no ser calzada, %d de sentido único. "
		     "(Tools/VerificarRedViaria.py da estos mismos números.)"),
		Nodos.Num(), Tramos.Num(), Cruces, Fondos, LargoTotal / 100000.0,
		Descartadas, Unicas);

	// Publish as IRoadQueryService
	if (UGameInstance* GI = W->GetGameInstance())
		if (UAlsasuaServiceRegistry* Reg = GI->GetSubsystem<UAlsasuaServiceRegistry>())
			Reg->Publicar(FName("Roads"), this);

	return Tramos.Num();
}

FVector UAlsasuaRedViaria::PosicionNodo(int32 Nodo) const
{
	return Nodos.IsValidIndex(Nodo) ? Nodos[Nodo] : FVector::ZeroVector;
}

int32 UAlsasuaRedViaria::TramoAleatorio(int32 Semilla) const
{
	if (Tramos.Num() == 0) return -1;
	FRandomStream Sorteo(Semilla * 2654435761u + 0x5EEDu);
	return Sorteo.RandRange(0, Tramos.Num() - 1);
}

int32 UAlsasuaRedViaria::SiguienteTramo(int32 TramoActual, int32 Semilla) const
{
	if (!Tramos.IsValidIndex(TramoActual)) return -1;

	const FTramoViario& T = Tramos[TramoActual];
	if (!Salidas.IsValidIndex(T.NodoB)) return -1;

	const TArray<int32>& Opciones = Salidas[T.NodoB];
	if (Opciones.Num() == 0) return -1;

	// Sin media vuelta salvo que no quede otra: es lo que separa "circular" de
	// "rebotar entre dos nodos". El fondo de saco sí la necesita.
	TArray<int32, TInlineAllocator<8>> Validas;
	for (int32 Idx : Opciones)
	{
		if (Tramos[Idx].NodoB != T.NodoA) Validas.Add(Idx);
	}

	FRandomStream Sorteo(Semilla * 2654435761u + TramoActual);
	if (Validas.Num() > 0) return Validas[Sorteo.RandRange(0, Validas.Num() - 1)];
	return Opciones[Sorteo.RandRange(0, Opciones.Num() - 1)];
}

int32 UAlsasuaRedViaria::NodoMasCercano(const FVector& Pos) const
{
	int32 Mejor = -1;
	float MejorDist = TNumericLimits<float>::Max();
	for (int32 i = 0; i < Nodos.Num(); ++i)
	{
		const float D = FVector::DistSquared2D(Nodos[i], Pos);
		if (D < MejorDist) { MejorDist = D; Mejor = i; }
	}
	return Mejor;
}

TArray<int32> UAlsasuaRedViaria::Ruta(int32 NodoInicio, int32 NodoFin) const
{
	TArray<int32> Salida;
	if (!Nodos.IsValidIndex(NodoInicio) || !Nodos.IsValidIndex(NodoFin)) return Salida;
	if (NodoInicio == NodoFin) return Salida;

	// A* sobre los tramos que SALEN de cada nodo. La cola es un array ordenado
	// por f: la red son ~2300 nodos, y un heap propio no compensa la línea extra.
	TArray<float> G;   G.Init(TNumericLimits<float>::Max(), Nodos.Num());
	TArray<int32> DeDonde; DeDonde.Init(-1, Nodos.Num());   // tramo por el que se llegó
	TArray<bool>  Cerrado; Cerrado.Init(false, Nodos.Num());

	const FVector Destino = Nodos[NodoFin];
	auto Heuristica = [&](int32 N) { return FVector::Dist2D(Nodos[N], Destino); };

	TArray<TPair<float, int32>> Abierta;   // (f, nodo)
	G[NodoInicio] = 0.f;
	Abierta.Add(TPair<float, int32>(Heuristica(NodoInicio), NodoInicio));

	while (Abierta.Num() > 0)
	{
		int32 IdxMejor = 0;
		for (int32 i = 1; i < Abierta.Num(); ++i)
		{
			if (Abierta[i].Key < Abierta[IdxMejor].Key) IdxMejor = i;
		}
		const int32 Actual = Abierta[IdxMejor].Value;
		Abierta.RemoveAtSwap(IdxMejor, EAllowShrinking::No);

		if (Actual == NodoFin) break;
		if (Cerrado[Actual]) continue;
		Cerrado[Actual] = true;

		if (!Salidas.IsValidIndex(Actual)) continue;
		for (int32 IdxTramo : Salidas[Actual])
		{
			const FTramoViario& T = Tramos[IdxTramo];
			if (Cerrado[T.NodoB]) continue;

			const float Candidato = G[Actual] + T.LargoCm;
			if (Candidato >= G[T.NodoB]) continue;

			G[T.NodoB] = Candidato;
			DeDonde[T.NodoB] = IdxTramo;
			Abierta.Add(TPair<float, int32>(Candidato + Heuristica(T.NodoB), T.NodoB));
		}
	}

	if (DeDonde[NodoFin] < 0) return Salida;   // sin camino

	for (int32 N = NodoFin; N != NodoInicio && DeDonde[N] >= 0; N = Tramos[DeDonde[N]].NodoA)
	{
		Salida.Add(DeDonde[N]);
	}
	Algo::Reverse(Salida);
	return Salida;
}

TArray<FVector> UAlsasuaRedViaria::PuntosDeRuta(const TArray<int32>& RutaTramos) const
{
	TArray<FVector> Puntos;
	if (RutaTramos.Num() == 0) return Puntos;

	for (int32 i = 0; i < RutaTramos.Num(); ++i)
	{
		if (!Tramos.IsValidIndex(RutaTramos[i])) continue;
		const FTramoViario& T = Tramos[RutaTramos[i]];
		const FVector A = Nodos[T.NodoA];
		const FVector B = Nodos[T.NodoB];

		const FVector Dir = (B - A).GetSafeNormal();
		const FVector Perp(-Dir.Y, Dir.X, 0.f);
		const FVector Carril = Perp * (T.AnchoCm * 0.25f);

		if (i == 0) Puntos.Add(A + Carril);
		Puntos.Add(B + Carril);
	}
	return Puntos;
}

// ── IRoadQueryService ───────────────────────────────────────────────────────

bool UAlsasuaRedViaria::IsRoadAt(const FVector& Location, float Radius) const
{
	const int32 Nearest = NodoMasCercano(Location);
	if (Nearest < 0) return false;
	return FVector::DistSquared(Location, Nodos[Nearest]) < FMath::Square(Radius);
}

FVector UAlsasuaRedViaria::GetNearestRoadPoint(const FVector& Location) const
{
	const int32 Nearest = NodoMasCercano(Location);
	return Nearest >= 0 ? Nodos[Nearest] : Location;
}

bool UAlsasuaRedViaria::GetRoadDirection(const FVector& Location, FVector& OutDirection) const
{
	const int32 Nearest = NodoMasCercano(Location);
	if (Nearest < 0 || !Salidas.IsValidIndex(Nearest) || Salidas[Nearest].Num() == 0) return false;

	const int32 FirstTramo = Salidas[Nearest][0];
	if (!Tramos.IsValidIndex(FirstTramo)) return false;
	const FTramoViario& T = Tramos[FirstTramo];
	OutDirection = (Nodos[T.NodoB] - Nodos[T.NodoA]).GetSafeNormal();
	return true;
}

float UAlsasuaRedViaria::GetSpeedLimitAt(const FVector& Location) const
{
	// All roads are 50 km/h urban limit unless marked as highway
	const int32 Nearest = NodoMasCercano(Location);
	if (Nearest < 0) return 50.f;

	for (const int32 Ti : Salidas[Nearest])
	{
		if (Tramos.IsValidIndex(Ti))
		{
			const FString& Tipo = Tramos[Ti].Tipo;
			if (Tipo.Contains(TEXT("motorway")) || Tipo.Contains(TEXT("trunk"))) return 120.f;
			if (Tipo.Contains(TEXT("primary")))   return 90.f;
			if (Tipo.Contains(TEXT("secondary")))  return 70.f;
		}
	}
	return 50.f;
}

bool UAlsasuaRedViaria::IsOneWayAt(const FVector& Location) const
{
	const int32 Nearest = NodoMasCercano(Location);
	if (Nearest < 0) return false;

	// Check if any outgoing tramo from this node is oneway=yes
	for (const int32 Ti : Salidas[Nearest])
	{
		if (Tramos.IsValidIndex(Ti))
		{
			const FString& Tipo = Tramos[Ti].Tipo;
			if (Tipo.Contains(TEXT("oneway"))) return true;
		}
	}
	return false;
}
