// HerrikoPlazaGenerator.cpp
#include "HerrikoPlazaGenerator.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "GeoDataAlsasua.h"

AHerrikoPlazaGenerator::AHerrikoPlazaGenerator()
{
	PrimaryActorTick.bCanEverTick = false;
	MallaPlaza = CreateDefaultSubobject<UProceduralMeshComponent>(TEXT("MallaPlaza"));
	RootComponent = MallaPlaza;
	MallaPlaza->bUseAsyncCooking = true;
}

void AHerrikoPlazaGenerator::BeginPlay()
{
	Super::BeginPlay();
	Generar();
}

void AHerrikoPlazaGenerator::Generar()
{
	// Herriko Plaza está en el origen (0,0) en coordenadas de mundo.
	// Elevación: ~531.94m (CotaPlaza).
	SetActorLocation(FVector(0, 0, UAlsasuaGeoData::CotaPlazaCm));

	GenerarPavimento(MallaPlaza);
	GenerarFuente(MallaPlaza);
	GenerarBancos(MallaPlaza);
}

void AHerrikoPlazaGenerator::GenerarPavimento(UProceduralMeshComponent* Malla)
{
	// Plaza rectangular irregular ~60m x 40m con adoquines de piedra.
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FColor> C;

	const FColor ColorAdoquin(150, 142, 128); // piedra caliza clara
	const float ZBase = 10.f; // ligeramente elevada sobre el terreno

	// Forma irregular de la plaza (aproximación basada en datos OSM).
	TArray<FVector2D> PlazaPoints = {
		FVector2D(-3000, -2000), FVector2D(3000, -2000),
		FVector2D(3200, -1800), FVector2D(3400, 0),
		FVector2D(3200, 1800), FVector2D(3000, 2000),
		FVector2D(-3000, 2000), FVector2D(-3200, 1800),
		FVector2D(-3400, 0), FVector2D(-3200, -1800)
	};

	const int32 Np = PlazaPoints.Num();
	for (const FVector2D& P : PlazaPoints)
	{
		V.Add(FVector(P.X, P.Y, ZBase));
		N.Add(FVector::UpVector);
		UV.Add(P / 1000.f); // UV tileado
		C.Add(ColorAdoquin);
	}

	// Triangulación simple: fan desde el centro.
	const FVector2D Centro(0, 0);
	const int32 CentroIdx = V.Num();
	V.Add(FVector(Centro.X, Centro.Y, ZBase));
	N.Add(FVector::UpVector);
	UV.Add(FVector2D(0.5f, 0.5f));
	C.Add(ColorAdoquin);

	for (int32 i = 0; i < Np; ++i)
	{
		T.Add(CentroIdx);
		T.Add(i);
		T.Add((i + 1) % Np);
	}

	TArray<FProcMeshTangent> Tang;
	Malla->CreateMeshSection(0, V, T, N, UV, C, Tang, true);

	static UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
	if (Mat) Malla->SetMaterial(0, Mat);
}

void AHerrikoPlazaGenerator::GenerarFuente(UProceduralMeshComponent* Malla)
{
	// Fuente circular en el centro de la plaza.
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FColor> C;

	const FColor ColorFuente(130, 135, 140); // piedra gris
	const FColor ColorAgua(60, 100, 160);
	const float Radio = 300.f; // 3m radio
	const float AlturaBase = 10.f;
	const float AlturaBorde = 80.f;
	const float AlturaAgua = 60.f;
	const int32 Segmentos = 24;

	// Base circular (cilindro).
	for (int32 i = 0; i < Segmentos; ++i)
	{
		const float A0 = (float)i / Segmentos * 2.f * PI;
		const float A1 = (float)(i + 1) / Segmentos * 2.f * PI;
		const FVector2D P0(FMath::Cos(A0) * Radio, FMath::Sin(A0) * Radio);
		const FVector2D P1(FMath::Cos(A1) * Radio, FMath::Sin(A1) * Radio);

		const int32 b = V.Num();
		// Cara exterior.
		V.Add(FVector(P0.X, P0.Y, AlturaBase));
		V.Add(FVector(P1.X, P1.Y, AlturaBase));
		V.Add(FVector(P1.X, P1.Y, AlturaBorde));
		V.Add(FVector(P0.X, P0.Y, AlturaBorde));
		FVector Nrm = FVector(P0.X, P0.Y, 0).GetSafeNormal();
		for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(ColorFuente); }
		UV.Add({0,0}); UV.Add({1,0}); UV.Add({1,1}); UV.Add({0,1});
		T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);

		// Superficie del agua.
		const int32 bw = V.Num();
		V.Add(FVector(0, 0, AlturaAgua));
		V.Add(FVector(P0.X, P0.Y, AlturaAgua));
		V.Add(FVector(P1.X, P1.Y, AlturaAgua));
		for (int32 k = 0; k < 3; ++k) { N.Add(FVector::UpVector); C.Add(ColorAgua); }
		UV.Add({0.5f, 0.5f}); UV.Add({0.5f + FMath::Cos(A0) * 0.5f, 0.5f + FMath::Sin(A0) * 0.5f});
		UV.Add({0.5f + FMath::Cos(A1) * 0.5f, 0.5f + FMath::Sin(A1) * 0.5f});
		T.Add(bw); T.Add(bw+1); T.Add(bw+2);
	}

	// Columna central.
	const float ColRadio = 40.f;
	const float ColAltura = 200.f;
	for (int32 i = 0; i < Segmentos; ++i)
	{
		const float A0 = (float)i / Segmentos * 2.f * PI;
		const float A1 = (float)(i + 1) / Segmentos * 2.f * PI;
		const FVector2D P0(FMath::Cos(A0) * ColRadio, FMath::Sin(A0) * ColRadio);
		const FVector2D P1(FMath::Cos(A1) * ColRadio, FMath::Sin(A1) * ColRadio);

		const int32 b = V.Num();
		V.Add(FVector(P0.X, P0.Y, AlturaAgua));
		V.Add(FVector(P1.X, P1.Y, AlturaAgua));
		V.Add(FVector(P1.X, P1.Y, AlturaAgua + ColAltura));
		V.Add(FVector(P0.X, P0.Y, AlturaAgua + ColAltura));
		FVector Nrm = FVector(P0.X, P0.Y, 0).GetSafeNormal();
		for (int32 k = 0; k < 4; ++k) { N.Add(Nrm); C.Add(ColorFuente); }
		UV.Add({0,0}); UV.Add({1,0}); UV.Add({1,1}); UV.Add({0,1});
		T.Add(b); T.Add(b+2); T.Add(b+1); T.Add(b); T.Add(b+3); T.Add(b+2);
	}

	TArray<FProcMeshTangent> Tang;
	Malla->CreateMeshSection(1, V, T, N, UV, C, Tang, true);

	static UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_AguaRio.M_AguaRio"));
	if (Mat) Malla->SetMaterial(1, Mat);
}

void AHerrikoPlazaGenerator::GenerarBancos(UProceduralMeshComponent* Malla)
{
	// 4 bancos de piedra alrededor de la fuente.
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FColor> C;

	const FColor ColorBanco(160, 155, 145);
	const float BancoDist = 600.f; // distancia al centro
	const float BancoLargo = 200.f;
	const float BancoAncho = 50.f;
	const float BancoAlto = 50.f;
	const float ZBase = 10.f;

	for (int32 i = 0; i < 4; ++i)
	{
		const float Angulo = i * PI * 0.5f + PI * 0.25f; // 45°, 135°, 225°, 315°
		const FVector Centro(FMath::Cos(Angulo) * BancoDist, FMath::Sin(Angulo) * BancoDist, ZBase);
		const FVector Dir(FMath::Cos(Angulo), FMath::Sin(Angulo), 0);
		const FVector Lateral(-Dir.Y, Dir.X, 0);

		const int32 b = V.Num();
		// Asiento (plano horizontal a altura BancoAlto).
		V.Add(Centro + Lateral * BancoLargo * 0.5f - Dir * BancoAncho * 0.5f);
		V.Add(Centro - Lateral * BancoLargo * 0.5f - Dir * BancoAncho * 0.5f);
		V.Add(Centro - Lateral * BancoLargo * 0.5f + Dir * BancoAncho * 0.5f);
		V.Add(Centro + Lateral * BancoLargo * 0.5f + Dir * BancoAncho * 0.5f);
		for (int32 k = 0; k < 4; ++k)
		{
			FVector& Vert = V[b + k];
			Vert.Z = ZBase + BancoAlto;
			N.Add(FVector::UpVector);
			C.Add(ColorBanco);
		}
		UV.Add({0,0}); UV.Add({1,0}); UV.Add({1,0.2f}); UV.Add({0,0.2f});
		T.Add(b); T.Add(b+1); T.Add(b+2); T.Add(b); T.Add(b+2); T.Add(b+3);
	}

	TArray<FProcMeshTangent> Tang;
	Malla->CreateMeshSection(2, V, T, N, UV, C, Tang, true);
}
