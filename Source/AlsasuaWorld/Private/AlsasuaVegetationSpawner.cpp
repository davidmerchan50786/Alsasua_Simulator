#include "AlsasuaVegetationSpawner.h"
#include "ProceduralMeshComponent.h"
#include "GeoDataAlsasua.h"
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

	UMaterialInterface* MatHierba = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Arbol.M_Arbol"));
	UMaterialInterface* MatArbusto = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Arbol.M_Arbol"));

	// Collect all greenspace polygons
	TArray<TArray<FVector>> Polygons;
	for (const auto& Item : Items)
	{
		const auto& O = Item->AsObject();
		if (!O.IsValid()) continue;

		const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
		if (!O->TryGetArrayField(TEXT("points"), Points) || !Points || Points->Num() < 3) continue;

		TArray<FVector> Poly;
		for (const auto& Pv : *Points)
		{
			const auto& Po = Pv->AsObject();
			if (!Po.IsValid()) continue;
			const FVector M = UAlsasuaGeoData::RelLocalToUE5(FVector(Po->GetNumberField(TEXT("x")), 0.0, Po->GetNumberField(TEXT("z"))));
			Poly.Add(FVector(M.X, M.Y, 0));
		}
		if (Poly.Num() >= 3) Polygons.Add(Poly);
	}

	if (Polygons.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Vegetacion] No se encontraron polígonos de zonas verdes"));
		return 0;
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

	AActor* HierbaActor = World->SpawnActor<AActor>(AActor::StaticClass(), FVector(MinX, MinY, 0), FRotator::ZeroRotator);
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

			float Rand = Rng.FRand();

			if (Rand < DensidadHierba * 0.8f && HierbaCount < MaxHierba)
			{
				// Grass blade (small triangle)
				float Scale = Rng.FRandRange(20.f, 50.f);
				int32 BV = V.Num();
				FVector Base(TestPt.X, TestPt.Y, 0);
				V.Append({
					Base + FVector(-Scale * 0.3f, 0, 0),
					Base + FVector(Scale * 0.3f, 0, 0),
					Base + FVector(0, Rng.FRandRange(-Scale * 0.2f, Scale * 0.2f), Scale)
				});
				N.Append({FVector(0, -1, 0.3f).GetSafeNormal(), FVector(0, 1, 0.3f).GetSafeNormal(), FVector(0, 0, 1)});
				UV.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(0.5f, 1)});

				// Green variation
				uint8 Green = (uint8)Rng.RandRange(80, 140);
				C.Append({FColor(30, Green, 25), FColor(30, Green, 25), FColor(35, Green + 10, 30)});
				T.Append({BV, BV + 1, BV + 2});
				++HierbaCount;
			}
			else if (Rand < DensidadHierba * 0.8f + DensidadArbusto && ArbustoCount < MaxArbustos)
			{
				// Bush (small pyramid shape)
				float Scale = Rng.FRandRange(30.f, 80.f);
				int32 BV = V.Num();
				FVector Base(TestPt.X, TestPt.Y, 0);
				V.Append({
					Base + FVector(-Scale * 0.5f, -Scale * 0.5f, 0),
					Base + FVector(Scale * 0.5f, -Scale * 0.5f, 0),
					Base + FVector(Scale * 0.5f, Scale * 0.5f, 0),
					Base + FVector(-Scale * 0.5f, Scale * 0.5f, 0),
					Base + FVector(0, 0, Scale)
				});
				N.Append({FVector(0, -1, 0), FVector(1, 0, 0), FVector(0, 1, 0), FVector(-1, 0, 0), FVector(0, 0, 1)});
				for (int32 k = 0; k < 5; ++k) UV.Add(FVector2D(0, 0));
				uint8 Green = (uint8)Rng.RandRange(60, 110);
				FColor BushColor(25, Green, 20);
				for (int32 k = 0; k < 5; ++k) C.Add(BushColor);

				T.Append({BV, BV + 1, BV + 4});
				T.Append({BV + 1, BV + 2, BV + 4});
				T.Append({BV + 2, BV + 3, BV + 4});
				T.Append({BV + 3, BV, BV + 4});
				// Bottom face
				T.Append({BV, BV + 2, BV + 1});
				T.Append({BV, BV + 3, BV + 2});
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
