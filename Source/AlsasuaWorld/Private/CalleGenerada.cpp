// CalleGenerada.cpp
#include "CalleGenerada.h"
#include "ProceduralMeshComponent.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "CollisionQueryParams.h"
#include "MuestreadorAltura.h"
#include "TerrenoGenerado.h"

// Liang-Barsky: recorta el tramo [A,B] a Caja y añade los extremos al clip.
static void CliparTramo(const FVector2D& A, const FVector2D& B, const FBox2D& Caja, TArray<FVector2D>& Out)
{
	const FVector2D D = B - A;
	const float P[4] = { -D.X, D.X, -D.Y, D.Y };
	const float Q[4] = { A.X - Caja.Min.X, Caja.Max.X - A.X, A.Y - Caja.Min.Y, Caja.Max.Y - A.Y };
	float T0 = 0.f, T1 = 1.f;
	for (int32 i = 0; i < 4; ++i)
	{
		if (FMath::Abs(P[i]) < 1e-6f)
		{
			if (Q[i] < 0.f) return;
		}
		else
		{
			const float T = Q[i] / P[i];
			if (P[i] < 0.f) { if (T > T1) return; T0 = FMath::Max(T0, T); }
			else            { if (T < T0) return; T1 = FMath::Min(T1, T); }
		}
	}
	Out.Add(A + D * T0);
	if (T1 > T0) Out.Add(A + D * T1);
}

ACalleGenerada::ACalleGenerada()
{
	PrimaryActorTick.bCanEverTick = false;
	Malla = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Malla"));
	RootComponent = Malla;
	Malla->bUseAsyncCooking = true;
	Malla->SetCollisionProfileName(TEXT("BlockAll"));
	Tags.Add(TEXT("Calle"));
}

float ACalleGenerada::AlturaSuelo(const FVector2D& XY) const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	if (const UMuestreadorAltura* Muestreador = W->GetSubsystem<UMuestreadorAltura>())
	{
		const float Altura = Muestreador->AlturaMundo(FVector(XY.X, XY.Y, 0.f));
		if (!FMath::IsNearlyZero(Altura)) return Altura;
	}
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaCalle), true);
	Q.AddIgnoredActor(this);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceUp), FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceDown), ECC_Visibility, Q))
		return Hit.Location.Z;
	return 0.f;
}

void ACalleGenerada::Construir(const TArray<FVector2D>& P, float AnchoCm)
{
	// Fuera del terreno no hay malla: AlturaSuelo clampea al borde y la cinta flota
	// en el cielo. Se recorta la polilínea al rectángulo del terreno antes de extruir.
	TArray<FVector2D> Pc;
	bool bHayTerreno = false;
	{
		const UWorld* W = GetWorld();
		if (W)
		{
			ATerrenoGenerado* Terreno = nullptr;
			for (TActorIterator<ATerrenoGenerado> It(W); It; ++It) { Terreno = *It; break; }
			if (Terreno)
			{
				bHayTerreno = true;
				const FBox2D Caja = Terreno->BoundsXY();
				Pc.Reserve(P.Num());
				for (int32 i = 0; i + 1 < P.Num(); ++i) CliparTramo(P[i], P[i + 1], Caja, Pc);
				for (int32 i = Pc.Num() - 1; i > 0; --i)
					if (FVector2D::Distance(Pc[i], Pc[i - 1]) < 1.f) Pc.RemoveAt(i);
			}
		}
	}
	if (bHayTerreno) { if (Pc.Num() < 2) return; }
	else             { Pc = P; }
	const TArray<FVector2D>& Q = Pc;
	const int32 N = Q.Num();
	if (N < 2) return;
	const float Half = AnchoCm * 0.5f;
	const float Eps  = EpsilonCm;     // alzado sobre el terreno (anti z-fighting)
	const FVector Base = GetActorLocation();

	TArray<FVector> Verts;     Verts.Reserve(N * 2);
	TArray<FVector> Normales;  Normales.Reserve(N * 2);
	TArray<FVector2D> UVs;     UVs.Reserve(N * 2);
	TArray<int32> Tris;        Tris.Reserve((N - 1) * 6);
	TArray<FColor> Colores;    Colores.Reserve(N * 2);

	float RecorridoM = 0.f;
	for (int32 i = 0; i < N; ++i)
	{
		// dirección por diferencia central (miter sencillo)
		FVector2D dir;
		if      (i == 0)      dir = Q[1] - Q[0];
		else if (i == N - 1)  dir = Q[N - 1] - Q[N - 2];
		else                  dir = Q[i + 1] - Q[i - 1];
		dir = dir.GetSafeNormal();
		const FVector2D perp(-dir.Y, dir.X);

		const FVector2D Lxy = Q[i] + perp * Half;
		const FVector2D Rxy = Q[i] - perp * Half;
		const float zL = AlturaSuelo(Lxy) + Eps;
		const float zR = AlturaSuelo(Rxy) + Eps;

		Verts.Add(FVector(Lxy.X, Lxy.Y, zL) - Base);
		Verts.Add(FVector(Rxy.X, Rxy.Y, zR) - Base);
		Normales.Add(FVector::UpVector);
		Normales.Add(FVector::UpVector);
		Colores.Add(ColorBase);
		Colores.Add(ColorBase);

		if (i > 0) RecorridoM += FVector2D::Distance(Q[i], Q[i - 1]) / 100.f;
		UVs.Add(FVector2D(0.f, RecorridoM));
		UVs.Add(FVector2D(1.f, RecorridoM));

		if (i < N - 1)
		{
			const int32 b = i * 2;   // L=b, R=b+1, L'=b+2, R'=b+3
			Tris.Add(b + 0); Tris.Add(b + 2); Tris.Add(b + 1);   // cara hacia arriba (CCW desde arriba)
			Tris.Add(b + 1); Tris.Add(b + 2); Tris.Add(b + 3);
		}
	}

	TArray<FProcMeshTangent> Tangentes;
	Malla->CreateMeshSection(0, Verts, Tris, Normales, UVs, Colores, Tangentes, true);
}
