#include "AlsasuaBridgeGenerator.h"
#include "GeoDataAlsasua.h"
#include "ProceduralMeshComponent.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Materials/MaterialInterface.h"

static void LoadBridgePoints(TArray<TPair<FVector, FVector>>& OutCrossings)
{
	OutCrossings.Empty();

	// Load waterways
	TArray<TArray<FVector>> Rios;
	{
		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/waterways_unity.json"));
		FString Texto;
		if (!FFileHelper::LoadFileToString(Texto, *Ruta)) return;
		TArray<TSharedPtr<FJsonValue>> Items;
		const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
		if (!FJsonSerializer::Deserialize(R, Items)) return;
		for (const auto& Item : Items)
		{
			const auto& O = Item->AsObject();
			if (!O.IsValid()) continue;
			const TArray<TSharedPtr<FJsonValue>>* Pts = nullptr;
			if (!O->TryGetArrayField(TEXT("pts"), Pts) || !Pts || Pts->Num() < 6) continue;
			TArray<FVector> Rio;
			for (int32 k = 0; k + 2 < Pts->Num(); k += 3)
			{
				const double ux = (*Pts)[k]->AsNumber();
				const double uz = (*Pts)[k + 2]->AsNumber();
				const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector(ux, 0.0, uz));
				Rio.Add(FVector(M.X, M.Y, 0));
			}
			if (Rio.Num() >= 2) Rios.Add(Rio);
		}
	}

	// Load roads
	TArray<TArray<FVector>> Calles;
	{
		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/roads_unity.json"));
		FString Texto;
		if (!FFileHelper::LoadFileToString(Texto, *Ruta)) return;
		TArray<TSharedPtr<FJsonValue>> Items;
		const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
		if (!FJsonSerializer::Deserialize(R, Items)) return;
		for (const auto& Item : Items)
		{
			const auto& O = Item->AsObject();
			if (!O.IsValid()) continue;
			const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
			if (!O->TryGetArrayField(TEXT("points"), Points)) continue;
			TArray<FVector> Calle;
			for (const auto& Pv : *Points)
			{
				const auto& Po = Pv->AsObject();
				if (!Po.IsValid()) continue;
				const double ux = Po->GetNumberField(TEXT("x")) + UAlsasuaGeoData::OX;
				const double uz = Po->GetNumberField(TEXT("z")) + UAlsasuaGeoData::OZ;
				const FVector M = UAlsasuaGeoData::UnityaUnreal(FVector(ux, 0.0, uz));
				Calle.Add(FVector(M.X, M.Y, 0));
			}
			if (Calle.Num() >= 2) Calles.Add(Calle);
		}
	}

	// Find crossings
	const float Umbral = 300.f;
	for (const auto& Calle : Calles)
	{
		for (int32 i = 0; i < Calle.Num() - 1; ++i)
		{
			for (const auto& Rio : Rios)
			{
				for (int32 j = 0; j < Rio.Num() - 1; ++j)
				{
					FVector ClosestA, ClosestB;
					FMath::SegmentDistToSegment(Calle[i], Calle[i + 1], Rio[j], Rio[j + 1], ClosestA, ClosestB);
					if (FVector::Dist(ClosestA, ClosestB) < Umbral)
					{
						OutCrossings.Add(MakeTuple(ClosestA, ClosestB));
					}
				}
			}
		}
	}
}

static void BuildStoneArchBridge(UProceduralMeshComponent* PMC, float Length, float Width)
{
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FColor> C;
	TArray<FProcMeshTangent> Tan;

	const FColor StoneColor(175, 165, 150);
	const FColor StoneDark(130, 120, 110);
	const float DeckThickness = 30.f;
	const float ArchRadius = Length * 0.35f;
	const float ArchHeight = Length * 0.25f;
	const int32 ArchSegs = 12;
	const float BarrierH = 90.f;
	const float BarrierW = 12.f;

	// === Bridge deck (flat ribbon) ===
	int32 BaseV = V.Num();
	V.Add(FVector(-Length * 0.5f, -Width * 0.5f, 0));
	V.Add(FVector(Length * 0.5f, -Width * 0.5f, 0));
	V.Add(FVector(Length * 0.5f, Width * 0.5f, 0));
	V.Add(FVector(-Length * 0.5f, Width * 0.5f, 0));
	for (int32 k = 0; k < 4; ++k) { N.Add(FVector(0, 0, 1)); C.Add(StoneColor); }
	UV.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1)});
	T.Append({BaseV, BaseV + 1, BaseV + 2, BaseV, BaseV + 2, BaseV + 3});

	// === Arch underneath ===
	BaseV = V.Num();
	for (int32 i = 0; i <= ArchSegs; ++i)
	{
		float Angle = PI * i / ArchSegs;
		float X = -Length * 0.5f + Length * i / ArchSegs;
		float Z = -ArchHeight * FMath::Sin(Angle);

		V.Add(FVector(X, -Width * 0.4f, Z));
		V.Add(FVector(X, Width * 0.4f, Z));
		N.Add(FVector(0, FMath::Sin(Angle), FMath::Cos(Angle)));
		N.Add(FVector(0, -FMath::Sin(Angle), FMath::Cos(Angle)));
		UV.Add(FVector2D((float)i / ArchSegs, 0));
		UV.Add(FVector2D((float)i / ArchSegs, 1));
		C.Add(StoneDark);
		C.Add(StoneDark);
	}
	for (int32 i = 0; i < ArchSegs; ++i)
	{
		int32 L0 = BaseV + i * 2, L1 = L0 + 1;
		int32 R0 = BaseV + (i + 1) * 2, R1 = R0 + 1;
		T.Append({L0, R0, L1, L1, R0, R1});
	}

	// === Arch thickness (second ring offset Z) ===
	int32 ArchBaseV = V.Num();
	float ArchThick = 60.f;
	for (int32 i = 0; i <= ArchSegs; ++i)
	{
		float Angle = PI * i / ArchSegs;
		float X = -Length * 0.5f + Length * i / ArchSegs;
		float Z = -ArchHeight * FMath::Sin(Angle) - ArchThick;
		V.Add(FVector(X, -Width * 0.4f, Z));
		V.Add(FVector(X, Width * 0.4f, Z));
		N.Add(FVector(0, FMath::Sin(Angle), FMath::Cos(Angle)));
		N.Add(FVector(0, -FMath::Sin(Angle), FMath::Cos(Angle)));
		UV.Add(FVector2D((float)i / ArchSegs, 0));
		UV.Add(FVector2D((float)i / ArchSegs, 1));
		C.Add(StoneDark);
		C.Add(StoneDark);
	}
	for (int32 i = 0; i < ArchSegs; ++i)
	{
		int32 L0 = ArchBaseV + i * 2, L1 = L0 + 1;
		int32 R0 = ArchBaseV + (i + 1) * 2, R1 = R0 + 1;
		T.Append({L0, L1, R0, R0, L1, R1});
	}

	// === Barriers (simple walls on each side) ===
	auto AddBarrierWall = [&](float Side)
	{
		int32 BV = V.Num();
		float Z0 = 0, Z1 = BarrierH;
		float Y = Side * (Width * 0.5f - BarrierW * 0.5f);
		V.Add(FVector(-Length * 0.5f, Y - BarrierW * 0.5f, Z0));
		V.Add(FVector(Length * 0.5f, Y - BarrierW * 0.5f, Z0));
		V.Add(FVector(Length * 0.5f, Y + BarrierW * 0.5f, Z0));
		V.Add(FVector(-Length * 0.5f, Y + BarrierW * 0.5f, Z0));
		V.Add(FVector(-Length * 0.5f, Y - BarrierW * 0.5f, Z1));
		V.Add(FVector(Length * 0.5f, Y - BarrierW * 0.5f, Z1));
		V.Add(FVector(Length * 0.5f, Y + BarrierW * 0.5f, Z1));
		V.Add(FVector(-Length * 0.5f, Y + BarrierW * 0.5f, Z1));
		for (int32 k = 0; k < 8; ++k) { N.Add(FVector(0, Side > 0 ? 1 : -1, 0)); C.Add(StoneColor); }
		UV.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1),
		           FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1)});
		T.Append({BV, BV + 1, BV + 2, BV, BV + 2, BV + 3});
		T.Append({BV + 4, BV + 5, BV + 6, BV + 4, BV + 6, BV + 7});
	};
	AddBarrierWall(1.f);
	AddBarrierWall(-1.f);

	PMC->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
}

static void BuildIronTrussBridge(UProceduralMeshComponent* PMC, float Length, float Width)
{
	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FColor> C;
	TArray<FProcMeshTangent> Tan;

	const FColor IronColor(60, 60, 65);
	const FColor IronDark(40, 40, 45);
	const float DeckThick = 20.f;
	const float TrussH = 180.f;
	const float TrussThick = 8.f;
	const int32 Sections = 6;
	const float SectionLen = Length / Sections;

	// === Deck ===
	int32 BV = V.Num();
	V.Add(FVector(-Length * 0.5f, -Width * 0.5f, 0));
	V.Add(FVector(Length * 0.5f, -Width * 0.5f, 0));
	V.Add(FVector(Length * 0.5f, Width * 0.5f, 0));
	V.Add(FVector(-Length * 0.5f, Width * 0.5f, 0));
	for (int32 k = 0; k < 4; ++k) { N.Add(FVector(0, 0, 1)); C.Add(IronDark); }
	UV.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(1, 1), FVector2D(0, 1)});
	T.Append({BV, BV + 1, BV + 2, BV, BV + 2, BV + 3});

	// === Truss structure (Warren truss on both sides) ===
	auto AddTrussSide = [&](float SideY)
	{
		for (int32 s = 0; s < Sections; ++s)
		{
			float X0 = -Length * 0.5f + s * SectionLen;
			float X1 = X0 + SectionLen;
			float XMid = (X0 + X1) * 0.5f;
			float Y = SideY;
			int32 TBase = V.Num();

			// Bottom chord
			V.Add(FVector(X0, Y, 0));
			V.Add(FVector(X1, Y, 0));
			// Top chord
			V.Add(FVector(X0, Y, TrussH));
			V.Add(FVector(X1, Y, TrussH));
			// Middle diagonal
			V.Add(FVector(XMid, Y, TrussH * 0.5f));

			for (int32 k = 0; k < 5; ++k)
			{
				N.Add(FVector(0, SideY > 0 ? 1 : -1, 0));
				C.Add(s % 2 == 0 ? IronColor : IronDark);
			}
			UV.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1), FVector2D(1, 1), FVector2D(0.5f, 0.5f)});

			// Bottom chord triangle
			T.Append({TBase, TBase + 1, TBase + 2});
			// Top chord triangle
			T.Append({TBase + 2, TBase + 3, TBase + 0});
			// Diagonal braces
			T.Append({TBase, TBase + 4, TBase + 2});
			T.Append({TBase + 1, TBase + 4, TBase + 3});
		}
	};
	AddTrussSide(Width * 0.5f);
	AddTrussSide(-Width * 0.5f);

	// === Cross braces between trusses ===
	for (int32 s = 0; s <= Sections; s += 2)
	{
		float X = -Length * 0.5f + s * SectionLen;
		int32 CB = V.Num();
		V.Add(FVector(X, -Width * 0.5f, TrussH));
		V.Add(FVector(X, Width * 0.5f, TrussH));
		V.Add(FVector(X, -Width * 0.5f, TrussH + 30.f));
		V.Add(FVector(X, Width * 0.5f, TrussH + 30.f));
		for (int32 k = 0; k < 4; ++k) { N.Add(FVector(1, 0, 0)); C.Add(IronDark); }
		UV.Append({FVector2D(0, 0), FVector2D(1, 0), FVector2D(0, 1), FVector2D(1, 1)});
		T.Append({CB, CB + 1, CB + 2, CB + 1, CB + 3, CB + 2});
	}

	PMC->CreateMeshSection(0, V, T, N, UV, C, Tan, true);
}

bool UAlsasuaBridgeGenerator::GenerarPuentesMejorados()
{
	TArray<TPair<FVector, FVector>> Crossings;
	LoadBridgePoints(Crossings);

	if (Crossings.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Puentes] No se encontraron cruces río/calles"));
		return false;
	}

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
	if (!World)
	{
		UE_LOG(LogTemp, Error, TEXT("[Puentes] No se pudo obtener UWorld"));
		return false;
	}

	UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Muro_Piedra.M_Muro_Piedra"));
	if (!Mat) Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Edificio.M_Edificio"));

	int32 BridgesCreated = 0;
	for (int32 i = 0; i < Crossings.Num(); ++i)
	{
		const auto& Cross = Crossings[i];
		FVector MidPoint = (Cross.Key + Cross.Value) * 0.5f;
		float BridgeLen = FVector::Dist(Cross.Key, Cross.Value) + 600.f; // extend over banks
		float BridgeWidth = 800.f; // 8m wide

		FVector Dir = (Cross.Value - Cross.Key).GetSafeNormal();
		if (Dir.SizeSquared() < 0.01f) Dir = FVector::ForwardVector;
		FRotator Rot = Dir.Rotation();

		AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(), MidPoint, Rot);
		if (!Actor) continue;

#if WITH_EDITOR
		Actor->SetActorLabel(FString::Printf(TEXT("Puente_%d_%s"), i, (i % 2 == 0) ? TEXT("Piedra") : TEXT("Hierro")));
#endif
		Actor->Tags.Add(TEXT("Puente"));

		UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(Actor);
		PMC->RegisterComponent();
		PMC->SetupAttachment(Actor->GetRootComponent());
		PMC->SetMobility(EComponentMobility::Static);
		PMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		Actor->SetRootComponent(PMC);

		// Alternate stone arch and iron truss bridges
		if (i % 2 == 0)
		{
			BuildStoneArchBridge(PMC, BridgeLen, BridgeWidth);
		}
		else
		{
			BuildIronTrussBridge(PMC, BridgeLen, BridgeWidth);
		}

		if (Mat) PMC->SetMaterial(0, Mat);
		++BridgesCreated;
	}

	UE_LOG(LogTemp, Log, TEXT("[Puentes] %d puentes mejorados generados"), BridgesCreated);
	return BridgesCreated > 0;
}
