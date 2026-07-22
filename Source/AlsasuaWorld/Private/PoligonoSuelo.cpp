// PoligonoSuelo.cpp
#include "PoligonoSuelo.h"
#include "TrianguladorPoligono.h"
#include "ProceduralMeshComponent.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"

APoligonoSuelo::APoligonoSuelo()
{
	PrimaryActorTick.bCanEverTick = false;
	Malla = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Malla"));
	RootComponent = Malla;
	Malla->bUseAsyncCooking = true;
	Malla->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // suelo decorativo, no bloquea
	Tags.Add(TEXT("Suelo"));
}

float APoligonoSuelo::AlturaSuelo(const FVector2D& XY) const
{
	const UWorld* W = GetWorld();
	if (!W) return 0.f;
	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaSueloPoli), true);
	Q.AddIgnoredActor(this);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, 500000.f), FVector(XY.X, XY.Y, -500000.f), ECC_Visibility, Q))
		return Hit.Location.Z;
	return 0.f;
}

void APoligonoSuelo::Construir(const TArray<FVector2D>& In, FColor Color, float EpsilonCm)
{
	TArray<FVector2D> P = In;
	if (P.Num() > 1 && P[0].Equals(P.Last(), 1.f)) P.Pop();   // quitar cierre duplicado
	const int32 N = P.Num();
	if (N < 3) return;

	TArray<int32> CapTris;
	TrianguladorPoligono::Triangular(P, CapTris);
	if (CapTris.Num() < 3) return;

	const FVector Base = GetActorLocation();
	TArray<FVector> Verts;     Verts.Reserve(N);
	TArray<FVector> Normales;  Normales.Reserve(N);
	TArray<FVector2D> UVs;     UVs.Reserve(N);
	TArray<FColor> Colores;    Colores.Reserve(N);

	for (int32 i = 0; i < N; ++i)
	{
		const float z = AlturaSuelo(P[i]) + EpsilonCm;
		Verts.Add(FVector(P[i].X, P[i].Y, z) - Base);
		Normales.Add(FVector::UpVector);
		UVs.Add(P[i] / 100.f);   // UV en metros
		Colores.Add(Color);
	}

	TArray<FProcMeshTangent> Tangentes;
	Malla->CreateMeshSection(0, Verts, CapTris, Normales, UVs, Colores, Tangentes, false);
}
