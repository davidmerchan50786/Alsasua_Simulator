#include "AlsasuaRiverMeshGenerator.h"
#include "GeoDataAlsasua.h"
#include "ProceduralMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Engine/StaticMeshActor.h"
#include "Materials/MaterialInterface.h"
#include "EditorAssetLibrary.h"
#include "Engine/AssetLibrary.h"

static bool LoadWaterwayPoints(TArray<TArray<FVector>>& OutRivers)
{
	OutRivers.Empty();
	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/waterways_unity.json"));
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{
		UE_LOG(LogTemp, Error, TEXT("[Rio] No se pudo cargar waterways_unity.json"));
		return false;
	}

	TArray<TSharedPtr<FJsonValue>> Items;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Items))
	{
		UE_LOG(LogTemp, Error, TEXT("[Rio] Error parseando waterways JSON"));
		return false;
	}

	for (const auto& Item : Items)
	{
		const auto& O = Item->AsObject();
		if (!O.IsValid()) continue;

		const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
		if (!O->TryGetArrayField(TEXT("pts"), Pts) || !Pts || Pts->Num() < 6) continue;

		const double Width = O->HasField(TEXT("width")) ? O->GetNumberField(TEXT("width")) : 8.0;
		TArray<FVector> Puntos;
		for (int32 k = 0; k + 2 < Pts->Num(); k += 3)
		{
			const double ux = (*Pts)[k]->AsNumber();
			const double uz = (*Pts)[k + 2]->AsNumber();
			const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector(ux, 0.0, uz));
			Puntos.Add(FVector(M.X, M.Y, 0));
		}
		if (Puntos.Num() >= 2) OutRivers.Add(Puntos);
	}

	UE_LOG(LogTemp, Log, TEXT("[Rio] %d ríos cargados"), OutRivers.Num());
	return OutRivers.Num() > 0;
}

bool UAlsasuaRiverMeshGenerator::GenerarLechoRio()
{
	TArray<TArray<FVector>> Rios;
	if (!LoadWaterwayPoints(Rios)) return false;

	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game)
			{
				World = Ctx.World();
				break;
			}
		}
	}
	if (!World)
	{
		World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	}
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[Rio] No se pudo obtener UWorld"));
		return false;
	}

	UMaterialInterface* MatAgua = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_AguaRio.M_AguaRio"));
	UMaterialInterface* MatLecho = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Terreno_Calles.M_Terreno_Calles"));

	int32 RiosGenerados = 0;
	float ProfundidadRio = 200.f; // 2 metros bajo el nivel del agua

	for (int32 Idx = 0; Idx < Rios.Num(); ++Idx)
	{
		const TArray<FVector>& Pts = Rios[Idx];
		if (Pts.Num() < 2) continue;

		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Pts[0], FRotator::ZeroRotator);
		if (!Actor) continue;

#if WITH_EDITOR
		Actor->SetActorLabel(FString::Printf(TEXT("RioLecho_%d"), Idx));
#endif
		Actor->Tags.Add(TEXT("Rio"));

		// ProceduralMesh for riverbed (flat ribbon at river bottom)
		UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(Actor);
		PMC->RegisterComponent();
		PMC->SetupAttachment(Actor->GetRootComponent());
		PMC->SetMobility(EComponentMobility::Static);
		PMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Actor->SetRootComponent(PMC);

		TArray<FVector> Vertices;
		TArray<int32> Triangles;
		TArray<FVector> Normals;
		TArray<FVector2D> UVs;
		TArray<FColor> VertexColors;
		TArray<FProcMeshTangent> Tangents;

		float Acumulado = 0.f;

		for (int32 i = 0; i < Pts.Num(); ++i)
		{
			// Compute perpendicular direction
			FVector Dir;
			if (i < Pts.Num() - 1)
				Dir = (Pts[i + 1] - Pts[i]).GetSafeNormal();
			else if (i > 0)
				Dir = (Pts[i] - Pts[i - 1]).GetSafeNormal();
			else
				Dir = FVector::ForwardVector;

			FVector Perp = FVector(-Dir.Y, Dir.X, 0).GetSafeNormal();
			float HalfWidth = 400.f; // 4m half-width

						// Left bank, bottom
						Vertices.Add(Pts[i] + Perp * HalfWidth - FVector(0, 0, ProfundidadRio));
						Normals.Add(FVector(0, 0, 1));
						UVs.Add(FVector2D(Acumulado / 1000.f, 0));
						VertexColors.Add(FColor(80, 70, 60)); // Brown riverbed

						// Right bank, bottom
						Vertices.Add(Pts[i] - Perp * HalfWidth - FVector(0, 0, ProfundidadRio));
			Normals.Add(FVector(0, 0, 1));
			UVs.Add(FVector2D(Acumulado / 1000.f, 1));
			VertexColors.Add(FColor(80, 70, 60));

			if (i > 0)
			{
				int32 Base = (i - 1) * 2;
				int32 Curr = i * 2;
				Triangles.Append({Base, Curr, Base + 1, Base + 1, Curr, Curr + 1});
			}

			if (i < Pts.Num() - 1)
				Acumulado += FVector::Dist(Pts[i], Pts[i + 1]);
		}

		if (Vertices.Num() >= 3)
		{
			PMC->CreateMeshSection(0, Vertices, Triangles, Normals, UVs, VertexColors, Tangents, true);
			if (MatLecho) PMC->SetMaterial(0, MatLecho);
		}

		// Water surface ribbon at Z=0 (sea level for river = water surface)
		UProceduralMeshComponent* WaterPMC = NewObject<UProceduralMeshComponent>(Actor);
		WaterPMC->RegisterComponent();
		WaterPMC->SetupAttachment(Actor->GetRootComponent());
		WaterPMC->SetMobility(EComponentMobility::Static);
		WaterPMC->SetCastShadow(false);

		TArray<FVector> WV;
		TArray<int32> WT;
		TArray<FVector> WN;
		TArray<FVector2D> WUV;
		TArray<FColor> WC;
		TArray<FProcMeshTangent> WTan;

		Acumulado = 0.f;
		for (int32 i = 0; i < Pts.Num(); ++i)
		{
			FVector Dir;
			if (i < Pts.Num() - 1)
				Dir = (Pts[i + 1] - Pts[i]).GetSafeNormal();
			else if (i > 0)
				Dir = (Pts[i] - Pts[i - 1]).GetSafeNormal();
			else
				Dir = FVector::ForwardVector;

			FVector Perp = FVector(-Dir.Y, Dir.X, 0).GetSafeNormal();
			float HalfWidth = 380.f; // slightly narrower than riverbed

			// Left water surface (slightly above riverbed bottom)
			WV.Add(Pts[i] + Perp * HalfWidth + FVector(0, 0, -50.f));
			WN.Add(FVector(0, 0, 1));
			WUV.Add(FVector2D(Acumulado / 500.f, 0));
			WC.Add(FColor(30, 80, 120));

			// Right water surface
			WV.Add(Pts[i] - Perp * HalfWidth + FVector(0, 0, -50.f));
			WN.Add(FVector(0, 0, 1));
			WUV.Add(FVector2D(Acumulado / 500.f, 1));
			WC.Add(FColor(30, 80, 120));

			if (i > 0)
			{
				int32 Base = (i - 1) * 2;
				int32 Curr = i * 2;
				WT.Append({Base, Curr, Base + 1, Base + 1, Curr, Curr + 1});
			}

			if (i < Pts.Num() - 1)
				Acumulado += FVector::Dist(Pts[i], Pts[i + 1]);
		}

		if (WV.Num() >= 3)
		{
			WaterPMC->CreateMeshSection(0, WV, WT, WN, WUV, WC, WTan, true);
			if (MatAgua) WaterPMC->SetMaterial(0, MatAgua);
		}

		++RiosGenerados;
	}

	UE_LOG(LogTemp, Log, TEXT("[Rio] %d lechos de río generados"), RiosGenerados);
	return RiosGenerados > 0;
}

bool UAlsasuaRiverMeshGenerator::GenerarBancasRio()
{
	TArray<TArray<FVector>> Rios;
	if (!LoadWaterwayPoints(Rios)) return false;

	UWorld* World = nullptr;
	if (GEngine)
	{
		for (const FWorldContext& Ctx : GEngine->GetWorldContexts())
		{
			if (Ctx.WorldType == EWorldType::PIE || Ctx.WorldType == EWorldType::Game)
			{
				World = Ctx.World();
				break;
			}
		}
	}
	if (!World) World = GEditor ? GEditor->GetEditorWorldContext().World() : nullptr;
	if (!World) return false;

	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Muro_Piedra.M_Muro_Piedra"));
	if (!Mat) Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Edificio.M_Edificio"));

	int32 BancasGen = 0;
	float AlturaBanco = 150.f; // 1.5m above riverbed
	float Profundidad = 200.f;

	for (int32 Idx = 0; Idx < Rios.Num(); ++Idx)
	{
		const TArray<FVector>& Pts = Rios[Idx];
		if (Pts.Num() < 2) continue;

		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), Pts[0], FRotator::ZeroRotator);
		if (!Actor) continue;

#if WITH_EDITOR
		Actor->SetActorLabel(FString::Printf(TEXT("RioBank_%d"), Idx));
#endif
		Actor->Tags.Add(TEXT("RioBank"));

		UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(Actor);
		PMC->RegisterComponent();
		PMC->SetupAttachment(Actor->GetRootComponent());
		PMC->SetMobility(EComponentMobility::Static);
		Actor->SetRootComponent(PMC);

		TArray<FVector> V;
		TArray<int32> T;
		TArray<FVector> N;
		TArray<FVector2D> UV;
		TArray<FColor> C;
		TArray<FProcMeshTangent> Tan;

		// Left bank wall
		float Acumulado = 0.f;
		for (int32 i = 0; i < Pts.Num(); ++i)
		{
			FVector Dir;
			if (i < Pts.Num() - 1) Dir = (Pts[i + 1] - Pts[i]).GetSafeNormal();
			else if (i > 0) Dir = (Pts[i] - Pts[i - 1]).GetSafeNormal();
			else Dir = FVector::ForwardVector;

			FVector Perp = FVector(-Dir.Y, Dir.X, 0).GetSafeNormal();
			float WallOffset = 420.f;

			// Top of bank wall (ground level)
			V.Add(Pts[i] + Perp * WallOffset);
			N.Add(Perp);
			UV.Add(FVector2D(Acumulado / 200.f, 1));
			C.Add(FColor(140, 130, 115));

			// Bottom of bank wall
			V.Add(Pts[i] + Perp * WallOffset - FVector(0, 0, Profundidad));
			N.Add(Perp);
			UV.Add(FVector2D(Acumulado / 200.f, 0));
			C.Add(FColor(100, 95, 85));

			if (i > 0)
			{
				int32 Base = (i - 1) * 2;
				int32 Curr = i * 2;
				T.Append({Base, Curr, Base + 1, Base + 1, Curr, Curr + 1});
			}

			if (i < Pts.Num() - 1)
				Acumulado += FVector::Dist(Pts[i], Pts[i + 1]);
		}

		// Right bank wall
		Acumulado = 0.f;
		int32 RightOffset = V.Num();
		for (int32 i = 0; i < Pts.Num(); ++i)
		{
			FVector Dir;
			if (i < Pts.Num() - 1) Dir = (Pts[i + 1] - Pts[i]).GetSafeNormal();
			else if (i > 0) Dir = (Pts[i] - Pts[i - 1]).GetSafeNormal();
			else Dir = FVector::ForwardVector;

			FVector Perp = FVector(-Dir.Y, Dir.X, 0).GetSafeNormal();
			float WallOffset = 420.f;

			V.Add(Pts[i] - Perp * WallOffset);
			N.Add(-Perp);
			UV.Add(FVector2D(Acumulado / 200.f, 1));
			C.Add(FColor(140, 130, 115));

			V.Add(Pts[i] - Perp * WallOffset - FVector(0, 0, Profundidad));
			N.Add(-Perp);
			UV.Add(FVector2D(Acumulado / 200.f, 0));
			C.Add(FColor(100, 95, 85));

			if (i > 0)
			{
				int32 Base = RightOffset + (i - 1) * 2;
				int32 Curr = RightOffset + i * 2;
				T.Append({Base, Curr, Base + 1, Base + 1, Curr, Curr + 1});
			}

			if (i < Pts.Num() - 1)
				Acumulado += FVector::Dist(Pts[i], Pts[i + 1]);
		}

		if (V.Num() >= 4)
		{
			PMC->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
			if (Mat) PMC->SetMaterial(0, Mat);
		}

		++BancasGen;
	}

	UE_LOG(LogTemp, Log, TEXT("[Rio] %d bancas de río generadas"), BancasGen);
	return BancasGen > 0;
}
