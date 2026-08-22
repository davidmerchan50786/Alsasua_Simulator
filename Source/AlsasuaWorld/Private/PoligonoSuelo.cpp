// PoligonoSuelo.cpp
#include "PoligonoSuelo.h"
#include "TrianguladorPoligono.h"
#include "ProceduralMeshComponent.h"
#include "GeoDataAlsasua.h"
#include "Engine/World.h"
#include "CollisionQueryParams.h"
#include "MuestreadorAltura.h"

APoligonoSuelo::APoligonoSuelo()
{
	PrimaryActorTick.bCanEverTick = false;
	Malla = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("Malla"));
	RootComponent = Malla;
	Malla->bUseAsyncCooking = true;
	Malla->SetCollisionEnabled(ECollisionEnabled::NoCollision);   // suelo decorativo, no bloquea
	Tags.Add(TEXT("Suelo"));
}

bool APoligonoSuelo::AlturaSuelo(const FVector2D& XY, float& OutZ) const
{
	OutZ = 0.f;
	const UWorld* W = GetWorld();
	if (!W) return false;

	// El heightmap en memoria primero: al drapear (fase 1b) sólo existe el
	// terreno, así que la cota bilineal es exactamente lo que encontraba el
	// trazo, sin consulta de física por vértice.
	if (const UMuestreadorAltura* Muestreador = W->GetSubsystem<UMuestreadorAltura>())
	{
		if (Muestreador->EstaDisponible() && UAlsasuaGeoData::DentroDelTerreno(FVector(XY.X, XY.Y, 0.f)))
		{
			OutZ = Muestreador->AlturaMundo(FVector(XY.X, XY.Y, 0.f));
			return true;
		}
	}

	FHitResult Hit;
	FCollisionQueryParams Q(SCENE_QUERY_STAT(AlturaSueloPoli), true);
	Q.AddIgnoredActor(this);
	if (W->LineTraceSingleByChannel(Hit, FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceUp), FVector(XY.X, XY.Y, UAlsasuaGeoData::TraceDown), ECC_Visibility, Q))
	{
		OutZ = Hit.Location.Z;
		return true;
	}
	// Devolvía 0 en este caso, que no es "no hay suelo" sino una altura de mundo
	// perfectamente válida (el cauce del Arakil anda por -1394). Los polígonos que
	// se salen del terreno — 3 zonas verdes tocan el borde — hundían ahí sus
	// vértices de fuera y levantaban un pico de ~550 m. Ahora se distingue.
	return false;
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

	// Primero muestreamos, y sólo con los vértices que han encontrado suelo. A los
	// que se salen del terreno les ponemos la media de los que sí, que deja el
	// borde plano en vez de despeñado. Si no hay ninguno, el polígono entero está
	// fuera del mundo y no se construye.
	TArray<float> Zs;   Zs.SetNumUninitialized(N);
	TArray<bool> HayZ;  HayZ.SetNumUninitialized(N);
	double Suma = 0.0;
	int32 Validos = 0;
	for (int32 i = 0; i < N; ++i)
	{
		float z = 0.f;
		HayZ[i] = AlturaSuelo(P[i], z);
		Zs[i] = z;
		if (HayZ[i]) { Suma += z; ++Validos; }
	}

	if (Validos == 0)
	{
		UE_LOG(LogTemp, Verbose, TEXT("[Suelos] polígono '%s' fuera del terreno; omitido."), *Tipo);
		return;
	}

	const float ZMedia = (float)(Suma / Validos);
	for (int32 i = 0; i < N; ++i)
	{
		const float z = (HayZ[i] ? Zs[i] : ZMedia) + EpsilonCm;
		Verts.Add(FVector(P[i].X, P[i].Y, z) - Base);
		Normales.Add(FVector::UpVector);
		UVs.Add(P[i] / 100.f);   // UV en metros
		Colores.Add(Color);
	}

	TArray<FProcMeshTangent> Tangentes;
	Malla->CreateMeshSection(0, Verts, CapTris, Normales, UVs, Colores, Tangentes, false);
}
