// CargadorPuentes.cpp
#include "CargadorPuentes.h"
#include "ProceduralMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "GeoDataAlsasua.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UCargadorPuentes::OnWorldBeginPlay(UWorld& InWorld)
{
	Super::OnWorldBeginPlay(InWorld);
	// Se ejecuta desde DirectorArranque::IniciarConstruccion().
}

int32 UCargadorPuentes::GenerarPuentes()
{
	DetectarCruces();
	for (const FPuenteData& P : PuentesEncontrados)
	{
		SpawnPuente(P);
	}
	UE_LOG(LogTemp, Log, TEXT("[Puentes] %d puentes generados"), PuentesEncontrados.Num());
	return PuentesEncontrados.Num();
}

void UCargadorPuentes::DetectarCruces()
{
	PuentesEncontrados.Empty();

	// Cargar waterways (formato: "pts" array plano [x,y,z, x,y,z, ...]).
	{
		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/waterways_unity.json"));
		FString Texto;
		if (!FFileHelper::LoadFileToString(Texto, *Ruta)) return;

		TArray<TSharedPtr<FJsonValue>> WItems;
		const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
		if (!FJsonSerializer::Deserialize(R, WItems)) return;

		for (const auto& Item : WItems)
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

	// Cargar roads (formato: "points":[{"x":..,"z":..}] ).
	{
		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/roads_unity.json"));
		FString Texto;
		if (!FFileHelper::LoadFileToString(Texto, *Ruta)) return;

		TArray<TSharedPtr<FJsonValue>> RItems;
		const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
		if (!FJsonSerializer::Deserialize(R, RItems)) return;

		for (const auto& Item : RItems)
		{
			const auto& O = Item->AsObject();
			if (!O.IsValid()) continue;
			const TArray<TSharedPtr<FJsonValue>>* Points = nullptr;
			if (!O->TryGetArrayField(TEXT("points"), Points)) continue;

			const FString Nombre = O->HasField(TEXT("name")) ? O->GetStringField(TEXT("name")) : FString();
			TArray<FVector> Calle;
			for (const auto& Pv : *Points)
			{
				const auto& Po = Pv->AsObject();
				if (!Po.IsValid()) continue;
			const FVector M = UAlsasuaGeoData::RelLocalToUE5(FVector(Po->GetNumberField(TEXT("x")), 0.0, Po->GetNumberField(TEXT("z"))));
				Calle.Add(FVector(M.X, M.Y, UAlsasuaGeoData::AlturaSueloUE5(GetWorld(), M.X, M.Y)));
			}
			if (Calle.Num() >= 2) Calles.Add(MakeTuple(Calle, Nombre));
		}
	}

	// Intersección: cada segmento de cada calle vs cada segmento de cada río.
	const float Umbral = 300.f; // 3 metros de tolerancia.
	for (const auto& CallePair : Calles)
	{
		const TArray<FVector>& Calle = CallePair.Key;
		for (int32 i = 0; i < Calle.Num() - 1; ++i)
		{
			for (const TArray<FVector>& Rio : Rios)
			{
				for (int32 j = 0; j < Rio.Num() - 1; ++j)
				{
					// Segmento-segmento distance.
					const FVector& A = Calle[i]; const FVector& B = Calle[i+1];
					const FVector& C = Rio[j];   const FVector& D = Rio[j+1];

					// Punto medio aproximado del cruce.
					FVector ClosestA, ClosestB;
					FMath::SegmentDistToSegment(A, B, C, D, ClosestA, ClosestB);
					const float Dist = FVector::Dist(ClosestA, ClosestB);

					if (Dist < Umbral)
					{
						const FVector Centro = (ClosestA + ClosestB) * 0.5f;
						const FVector Dir = (B - A).GetSafeNormal();
						const float Ancho = FVector::Dist(A, B) * 0.3f; // estimación
						const float Largo = Dist + 400.f; // extender un poco sobre las orillas

						FPuenteData PD;
						PD.PosicionMundo = Centro;
						PD.Direccion = Dir;
						PD.AnchoCalzada = FMath::Max(Ancho, 600.f);
						PD.LargoPuente = Largo;
						PD.NombreCalle = CallePair.Value;
						PuentesEncontrados.Add(PD);
					}
				}
			}
		}
	}
}

void UCargadorPuentes::SpawnPuente(const FPuenteData& Data)
{
	UWorld* World = GetWorld();
	if (!World) return;

	AActor* Actor = World->SpawnActor<AActor>(AActor::StaticClass(),
		Data.PosicionMundo, Data.Direccion.Rotation());
	if (!Actor) return;

	UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(Actor);
	PMC->RegisterComponent();
	PMC->SetupAttachment(Actor->GetRootComponent());
	PMC->bUseAsyncCooking = true;
	PMC->SetMobility(EComponentMobility::Static);
	PMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PMC->SetCollisionObjectType(ECC_WorldStatic);

	// Generar malla de puente: cubo aplanado con barandillas.
	const float L = Data.LargoPuente;
	const float W = Data.AnchoCalzada;
	const float H = 40.f; // 40cm grosor del tablero
	const float BarandaH = 100.f;
	const float ZBase = 200.f; // elevación sobre el nivel del agua

	TArray<FVector> V;
	TArray<int32> T;
	TArray<FVector> N;
	TArray<FVector2D> UV;
	TArray<FColor> C;

	const FColor ColorPuente(100, 100, 105); // hormigón gris

	// Tablero (cubo aplanado).
	auto AddBox = [&](FVector Center, FVector Extent, FColor Col)
	{
		const int32 b = V.Num();
		FVector X(Extent.X, 0, 0), Y(0, Extent.Y, 0), Z(0, 0, Extent.Z);
		// 8 vértices.
		V.Add(Center - X - Y - Z); V.Add(Center + X - Y - Z);
		V.Add(Center + X + Y - Z); V.Add(Center - X + Y - Z);
		V.Add(Center - X - Y + Z); V.Add(Center + X - Y + Z);
		V.Add(Center + X + Y + Z); V.Add(Center - X + Y + Z);
		for (int32 k = 0; k < 8; ++k) { N.Add(FVector::ZeroVector); C.Add(Col); }
		// 12 triángulos (6 caras).
		int32 F[6][4] = {{0,1,2,3},{4,5,6,7},{0,1,5,4},{2,3,7,6},{0,3,7,4},{1,2,6,5}};
		for (int32 f = 0; f < 6; ++f)
		{
			T.Add(b+F[f][0]); T.Add(b+F[f][1]); T.Add(b+F[f][2]);
			T.Add(b+F[f][0]); T.Add(b+F[f][2]); T.Add(b+F[f][3]);
		}
	};

	AddBox(FVector(0, 0, ZBase), FVector(L * 0.5f, W * 0.5f, H * 0.5f), ColorPuente);

	// Barandillas laterales.
	const float BarW = 15.f;
	AddBox(FVector(0, W * 0.5f - BarW, ZBase + H * 0.5f + BarandaH * 0.5f),
		FVector(L * 0.5f, BarW * 0.5f, BarandaH * 0.5f), FColor(80, 80, 85));
	AddBox(FVector(0, -W * 0.5f + BarW, ZBase + H * 0.5f + BarandaH * 0.5f),
		FVector(L * 0.5f, BarW * 0.5f, BarandaH * 0.5f), FColor(80, 80, 85));

	TArray<FProcMeshTangent> Tang;
	PMC->CreateMeshSection(0, V, T, N, UV, C, Tang, true);

	static UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Edificio.M_Edificio"));
	if (Mat) PMC->SetMaterial(0, Mat);

	Actor->SetRootComponent(PMC);
	Actor->Tags.Add(TEXT("Puente"));
}
