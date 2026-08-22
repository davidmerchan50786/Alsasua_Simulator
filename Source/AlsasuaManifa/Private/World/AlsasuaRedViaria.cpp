// AlsasuaRedViaria.cpp
#include "World/AlsasuaRedViaria.h"
#include "CargarJsonComun.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"

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
