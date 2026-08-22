// AlsasuaMuros.cpp
#include "World/AlsasuaMuros.h"
#include "CargarJsonComun.h"

namespace
{
	TArray<AlsasuaMuros::FMuro> Muros;
	bool bLeidos = false;

	void Cargar()
	{
		if (bLeidos) return;
		bLeidos = true;

		TArray<TSharedPtr<FJsonValue>> Edificios;
		if (!JsonDatos::CargarArray(TEXT("Datos/buildings_final.json"), Edificios, { TEXT("buildings") }))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Muros] sin footprints: nada donde colgar fachada"));
			return;
		}

		for (const TSharedPtr<FJsonValue>& EV : Edificios)
		{
			const TSharedPtr<FJsonObject> Edif = EV->AsObject();
			if (!Edif.IsValid()) continue;

			const TArray<TSharedPtr<FJsonValue>>* Verts = nullptr;
			if (!Edif->TryGetArrayField(TEXT("vertices"), Verts) || !Verts || Verts->Num() < 3) continue;

			const int32 Id = Edif->HasField(TEXT("id")) ? Edif->GetIntegerField(TEXT("id")) : -1;
			FString Barrio;
			Edif->TryGetStringField(TEXT("barrio"), Barrio);

			TArray<FVector2D> Contorno;
			Contorno.Reserve(Verts->Num());
			FVector2D Centro(0.0f, 0.0f);
			for (const TSharedPtr<FJsonValue>& V : *Verts)
			{
				const TSharedPtr<FJsonObject> Vert = V->AsObject();
				if (!Vert.IsValid()) continue;
				const FVector2D P(Vert->GetNumberField(TEXT("x")), Vert->GetNumberField(TEXT("z")));
				Contorno.Add(P);
				Centro += P;
			}
			if (Contorno.Num() < 3) continue;
			Centro /= Contorno.Num();

			for (int32 v = 0; v < Contorno.Num(); ++v)
			{
				const FVector2D A = Contorno[v];
				const FVector2D B = Contorno[(v + 1) % Contorno.Num()];
				const float L = FVector2D::Distance(A, B);
				if (L < 0.5f) continue;   // vértices repetidos del trazado OSM

				const FVector2D Dir = (B - A) / L;
				FVector2D N(Dir.Y, -Dir.X);

				// Si la normal apunta al centroide es la de dentro: se le da la
				// vuelta. Sin esto, la mitad de lo que se cuelgue queda pintado
				// por el lado de dentro del muro.
				if (FVector2D::DotProduct(N, (A + B) * 0.5f - Centro) < 0.0f) N = -N;

				AlsasuaMuros::FMuro M;
				M.EdificioId = Id;
				M.Barrio = Barrio;
				M.A = A;
				M.B = B;
				M.LargoM = L;
				M.Fuera = N;
				M.Yaw = FMath::RadiansToDegrees(FMath::Atan2(N.Y, N.X));
				Muros.Add(MoveTemp(M));
			}
		}

		UE_LOG(LogTemp, Log, TEXT("[Muros] %d tramos de fachada de %d edificios"),
			Muros.Num(), Edificios.Num());
	}
}

const TArray<AlsasuaMuros::FMuro>& AlsasuaMuros::Todos()
{
	Cargar();
	return Muros;
}

void AlsasuaMuros::DeAlMenos(float LargoMinimoM, TArray<int32>& OutIndices)
{
	Cargar();
	OutIndices.Reset();
	for (int32 i = 0; i < Muros.Num(); ++i)
	{
		if (Muros[i].LargoM >= LargoMinimoM) OutIndices.Add(i);
	}
}

const AlsasuaMuros::FMuro* AlsasuaMuros::MasCercano(const FVector2D& PuntoRel, float RadioMaxM,
	float LargoMinimoM)
{
	Cargar();

	const float RadioMax2 = RadioMaxM * RadioMaxM;
	const FMuro* Mejor = nullptr;
	float MejorDist2 = RadioMax2;

	for (const FMuro& M : Muros)
	{
		if (M.LargoM < LargoMinimoM) continue;

		// Distancia al segmento, no al punto medio: una medianera de 40 m tiene
		// su punto medio a 20 m de una esquina que está pegada a la calle.
		const FVector2D AB = M.B - M.A;
		const float Len2 = AB.SizeSquared();
		const float T = (Len2 > KINDA_SMALL_NUMBER)
			? FMath::Clamp(FVector2D::DotProduct(PuntoRel - M.A, AB) / Len2, 0.0f, 1.0f)
			: 0.0f;
		const float D2 = FVector2D::DistSquared(PuntoRel, M.A + AB * T);

		if (D2 < MejorDist2) { MejorDist2 = D2; Mejor = &M; }
	}

	return Mejor;
}

void AlsasuaMuros::LimpiarCache()
{
	Muros.Empty();
	bLeidos = false;
}
