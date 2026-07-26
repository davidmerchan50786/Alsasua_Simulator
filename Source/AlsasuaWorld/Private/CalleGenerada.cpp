// CalleGenerada.cpp
#include "CalleGenerada.h"
#include "ProceduralMeshComponent.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

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
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaCalle), true);
	Q.AddIgnoredActor(this);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceUp), FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceDown), ECC_Visibility, Q))
		return Hit.Location.Z;
	return 0.f;
}

void ACalleGenerada::Construir(const TArray<FVector2D>& P, float AnchoCm)
{
	const int32 N = P.Num();
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
		if      (i == 0)      dir = P[1] - P[0];
		else if (i == N - 1)  dir = P[N - 1] - P[N - 2];
		else                  dir = P[i + 1] - P[i - 1];
		dir = dir.GetSafeNormal();
		const FVector2D perp(-dir.Y, dir.X);

		const FVector2D Lxy = P[i] + perp * Half;
		const FVector2D Rxy = P[i] - perp * Half;
		const float zL = AlturaSuelo(Lxy) + Eps;
		const float zR = AlturaSuelo(Rxy) + Eps;

		Verts.Add(FVector(Lxy.X, Lxy.Y, zL) - Base);
		Verts.Add(FVector(Rxy.X, Rxy.Y, zR) - Base);
		Normales.Add(FVector::UpVector);
		Normales.Add(FVector::UpVector);
		Colores.Add(ColorBase);
		Colores.Add(ColorBase);

		if (i > 0) RecorridoM += FVector2D::Distance(P[i], P[i - 1]) / 100.f;
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
