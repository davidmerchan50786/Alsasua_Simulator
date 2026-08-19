#include "AlsasuaVegetationSpawner.h"
#include "ProceduralMeshComponent.h"
#include "GeoDataAlsasua.h"
#include "MuestreadorAltura.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"
#include "Materials/MaterialInterface.h"

void UAlsasuaVegetationSpawner::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
}

int32 UAlsasuaVegetationSpawner::SembrarVegetacion()
{
	UWorld* World = GetWorld();
	if (!World) return 0;
	const UMuestreadorAltura* Alturas = World->GetSubsystem<UMuestreadorAltura>();

	// Load greenspaces data
	const FString RutaGS = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/greenspaces_unity.json"));
	FString TextoGS;
	if (!FFileHelper::LoadFileToString(TextoGS, *RutaGS))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Vegetacion] No se pudo cargar greenspaces_unity.json"));
		return 0;
	}

	TArray<TSharedPtr<FJsonValue>> Items;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(TextoGS);
	if (!FJsonSerializer::Deserialize(R, Items))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Vegetacion] Error parseando greenspaces JSON"));
		return 0;
	}

	// El césped tiene su material con el set Grass (color, normal, roughness y
	// AO); antes usaba el de árbol, que es hoja de copa y se veía plano en el
	// suelo. Si no está creado todavía, cae al de árbol como antes.
	UMaterialInterface* MatHierba = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Terreno_Hierba.M_Terreno_Hierba"));
	if (!MatHierba) MatHierba = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Arbol.M_Arbol"));

	UMaterialInterface* MatArbusto = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Arbol.M_Arbol"));

	// Collect all greenspace polygons
	TArray<TArray<FVector>> Polygons;
	for (const auto& Item : Items)
	{
		const auto& O = Item->AsObject();
		if (!O.IsValid()) continue;

		const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
		if (!O->TryGetArrayField(TEXT("poly"), Points) || !Points || Points->Num() < 6) continue;

		TArray<FVector> Poly;
		for (int32 i = 0; i + 1 < Points->Num(); i += 2)
		{
			// greenspaces_unity.json es ABSOLUTO en mundo Unity (x,z en metros).
			const FVector M = UAlsasuaGeoData::UnityaUnreal(
				FVector((*Points)[i]->AsNumber(), 0.0, (*Points)[i + 1]->AsNumber()));
			Poly.Add(FVector(M.X, M.Y, 0));
		}
		if (Poly.Num() >= 3) Polygons.Add(Poly);
	}

	if (Polygons.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Vegetacion] No se encontraron polígonos de zonas verdes"));
		return 0;
	}

	// Los cauces reales concentran arbusto ribereño; se usa una muestra ligera
	// para no convertir la siembra en una búsqueda cara contra todos los vértices.
	TArray<FVector2D> PuntosAgua;
	FString TextoAgua;
	if (FFileHelper::LoadFileToString(TextoAgua, *FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/waterways_unity.json"))))
	{
		TArray<TSharedPtr<FJsonValue>> Cauces;
		if (FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(TextoAgua), Cauces))
		{
			for (const TSharedPtr<FJsonValue>& Cauce : Cauces)
			{
				const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
				if (!Cauce->AsObject()->TryGetArrayField(TEXT("pts"), Pts) || !Pts) continue;
				for (int32 i = 0; i + 2 < Pts->Num(); i += 9)
				{
					const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector((*Pts)[i]->AsNumber(), 0.0, (*Pts)[i + 2]->AsNumber()));
					PuntosAgua.Add(FVector2D(M.X, M.Y));
				}
			}
		}
	}

	// Compute bounds of all polygons
	float MinX = FLT_MAX, MaxX = -FLT_MAX, MinY = FLT_MAX, MaxY = -FLT_MAX;
	for (const auto& Poly : Polygons)
	{
		for (const auto& V : Poly)
		{
			MinX = FMath::Min(MinX, V.X);
			MaxX = FMath::Max(MaxX, V.X);
			MinY = FMath::Min(MinY, V.Y);
			MaxY = FMath::Max(MaxY, V.Y);
		}
	}

	// Point-in-polygon test
	auto PointInPoly = [](const FVector& P, const TArray<FVector>& Poly) -> bool
	{
		int32 N = Poly.Num();
		bool bInside = false;
		for (int32 i = 0, j = N - 1; i < N; j = i++)
		{
			if ((Poly[i].Y > P.Y) != (Poly[j].Y > P.Y) &&
				P.X < (Poly[j].X - Poly[i].X) * (P.Y - Poly[i].Y) / (Poly[j].Y - Poly[i].Y) + Poly[i].X)
			{
				bInside = !bInside;
			}
		}
		return bInside;
	};

	// Los vértices se generan ya en coordenadas mundo; el actor debe quedarse en
	// origen para no sumar MinX/MinY una segunda vez y desplazar toda la vegetación.
	AActor* HierbaActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
	if (!HierbaActor) return 0;

#if WITH_EDITOR
	HierbaActor->SetActorLabel(TEXT("Vegetacion_Hierba"));
#endif

	UProceduralMeshComponent* HierbaPMC = NewObject<UProceduralMeshComponent>(HierbaActor);
	HierbaPMC->RegisterComponent();
	HierbaPMC->SetupAttachment(HierbaActor->GetRootComponent());
	HierbaPMC->SetMobility(EComponentMobility::Static);
	HierbaActor->SetRootComponent(HierbaPMC);

	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FColor> C;
	TArray<FProcMeshTangent> Tan;

	int32 HierbaCount = 0;
	int32 ArbustoCount = 0;
	const float StepSize = 150.f; // sample every 1.5m
	FRandomStream Rng(42);
	const auto CercaDeAgua = [&PuntosAgua](const FVector& P) -> float
	{
		float Dist2Min = 4000.f * 4000.f;
		for (const FVector2D& Agua : PuntosAgua) Dist2Min = FMath::Min(Dist2Min, FVector2D::DistSquared(FVector2D(P.X, P.Y), Agua));
		return 1.f - FMath::Clamp(FMath::Sqrt(Dist2Min) / 4000.f, 0.f, 1.f);
	};

	// Geometría helper: quad doble cara (visible desde ambos lados sin material two-sided).
	// Añade 4 verts + 4 triángulos (2 caras × 2 triángulos).
	auto AddQuad = [&](FVector BL, FVector BR, FVector TL, FVector TR,
	                   FVector Nrm, FColor Color)
	{
		const int32 B = V.Num();
		V.Append({BL, BR, TR, TL});
		N.Append({Nrm, Nrm, Nrm, Nrm});
		UV.Append({FVector2D(0,1), FVector2D(1,1), FVector2D(1,0), FVector2D(0,0)});
		C.Append({Color, Color, Color, Color});
		T.Append({B, B+2, B+1,  B, B+3, B+2});    // cara frontal
		T.Append({B, B+1, B+2,  B, B+2, B+3});    // cara trasera
		Tan.AddZeroed(4);
	};

	for (float X = MinX; X <= MaxX && HierbaCount < MaxHierba; X += StepSize)
	{
		for (float Y = MinY; Y <= MaxY && HierbaCount < MaxHierba; Y += StepSize)
		{
			FVector TestPt(X + Rng.FRandRange(-StepSize * 0.5f, StepSize * 0.5f),
			               Y + Rng.FRandRange(-StepSize * 0.5f, StepSize * 0.5f), 0);

			// Check if inside any greenspace polygon
			bool bInside = false;
			for (const auto& Poly : Polygons)
			{
				if (PointInPoly(TestPt, Poly))
				{
					bInside = true;
					break;
				}
			}
			if (!bInside) continue;

			const float Cluster = FMath::Lerp(0.35f, 1.f, FMath::Clamp((FMath::PerlinNoise2D(FVector2D(TestPt.X, TestPt.Y) / 5000.f) + 0.4f) / 0.8f, 0.f, 1.f));
			const float ProbHierba = DensidadHierba * 0.8f * Cluster;
			const float ProbArbusto = DensidadArbusto * FMath::Lerp(0.35f, 2.f, CercaDeAgua(TestPt));
			const float Rand = Rng.FRand();

			if (Rand < ProbHierba && HierbaCount < MaxHierba)
			{
				// Hierba: 2 quads perpendiculares en X (billboard cruzado).
				const float H    = Rng.FRandRange(30.f, 75.f);    // altura (cm)
				const float W    = H * 0.55f;                     // semi-ancho
				const float Yaw  = Rng.FRandRange(0.f, 360.f);   // rotación aleatoria
				const float CR   = FMath::DegreesToRadians(Yaw);
				const FVector XD(FMath::Cos(CR), FMath::Sin(CR), 0.f);
				const FVector YD(-XD.Y, XD.X, 0.f);
				const float Suelo = Alturas ? Alturas->AlturaMundo(TestPt) : 0.f;
				const FVector Base(TestPt.X, TestPt.Y, Suelo + 2.f);
				const FVector Top(0.f, 0.f, H);

				// Color verde con variación natural
				const uint8 G = (uint8)Rng.RandRange(82, 145);
				const FColor GrassColor(22 + Rng.RandRange(0,15), G, 18 + Rng.RandRange(0,12));

				// Quad 1 — alineado a XD
				AddQuad(Base - XD*W, Base + XD*W,
				        Base - XD*W + Top, Base + XD*W + Top,
				        YD, GrassColor);
				// Quad 2 — alineado a YD (perpendicular)
				AddQuad(Base - YD*W, Base + YD*W,
				        Base - YD*W + Top, Base + YD*W + Top,
				        XD, GrassColor);
				++HierbaCount;
			}
			else if (Rand < ProbHierba + ProbArbusto && ArbustoCount < MaxArbustos)
			{
				// Arbusto: 3 quads cruzados a 60° — aproxima una esfera de follaje.
				const float H    = Rng.FRandRange(45.f, 110.f);  // altura (cm)
				const float W    = H * 0.75f;                    // semi-ancho
				const float Yaw  = Rng.FRandRange(0.f, 360.f);
				const float Suelo = Alturas ? Alturas->AlturaMundo(TestPt) : 0.f;
				const FVector Base(TestPt.X, TestPt.Y, Suelo + 2.f);
				const FVector Top(0.f, 0.f, H);

				const uint8 G = (uint8)Rng.RandRange(60, 115);
				const FColor BushColor(20 + Rng.RandRange(0,12), G, 16 + Rng.RandRange(0,10));

				for (int32 Qi = 0; Qi < 3; ++Qi)
				{
					const float A  = FMath::DegreesToRadians(Yaw + Qi * 60.f);
					const FVector D(FMath::Cos(A), FMath::Sin(A), 0.f);
					const FVector Perp(-D.Y, D.X, 0.f);
					AddQuad(Base - D*W, Base + D*W,
					        Base - D*W + Top, Base + D*W + Top,
					        Perp, BushColor);
				}
				++ArbustoCount;
			}
		}
	}

	if (V.Num() >= 3)
	{
		HierbaPMC->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
		if (MatHierba) HierbaPMC->SetMaterial(0, MatHierba);
	}

	UE_LOG(LogTemp, Log, TEXT("[Vegetacion] %d hierbas + %d arbustos sembrados en %d zonas verdes"),
		HierbaCount, ArbustoCount, Polygons.Num());
	return HierbaCount + ArbustoCount;
}
