#include "TerrenoGenerado.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/World.h"

ATerrenoGenerado::ATerrenoGenerado()
{
	PrimaryActorTick.bCanEverTick = false;
	RootComponent = CreateDefaultSubobject<USceneComponent>(TEXT("Root"));
}

void ATerrenoGenerado::BeginPlay()
{
	Super::BeginPlay();
	if (AlturasRAW.Num() == 0)
	{
		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRAW);
		CargarRAW(Ruta);
		if (AlturasRAW.Num() > 0)
		{
			const FVector Centro = FVector(191800.0, 857000.0, 0.0);
			GenerarDesdeRAW(Ruta, ResolucionRAW, EscalaXY, EscalaZ, LocZ, Centro);
		}
	}
}

void ATerrenoGenerado::CargarRAW(const FString& Ruta)
{
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *Ruta))
	{
		UE_LOG(LogTemp, Error, TEXT("[Terreno] No se pudo leer %s"), *Ruta);
		return;
	}
	const int32 Esperado = ResolucionRAW * ResolucionRAW * 2;
	if (Bytes.Num() != Esperado)
	{
		UE_LOG(LogTemp, Error, TEXT("[Terreno] Tamaño %d != esperado %d (res %d)"), Bytes.Num(), Esperado, ResolucionRAW);
		return;
	}
	AlturasRAW.SetNumUninitialized(ResolucionRAW * ResolucionRAW);
	FMemory::Memcpy(AlturasRAW.GetData(), Bytes.GetData(), Esperado);
	UE_LOG(LogTemp, Log, TEXT("[Terreno] RAW cargado: %d bytes, %d×%d"), Bytes.Num(), ResolucionRAW, ResolucionRAW);
}

uint16 ATerrenoGenerado::LeerAltura(int32 PxX, int32 PxY) const
{
	PxX = FMath::Clamp(PxX, 0, ResolucionRAW - 1);
	PxY = FMath::Clamp(PxY, 0, ResolucionRAW - 1);
	return AlturasRAW[PxY * ResolucionRAW + PxX];
}

float ATerrenoGenerado::AlturaWorld(int32 PxX, int32 PxY) const
{
	return (float)LeerAltura(PxX, PxY) / 64.0f * (float)EscalaZ + (float)LocZ;
}

FVector ATerrenoGenerado::CalcNormal(int32 PxX, int32 PxY) const
{
	const float H = AlturaWorld(PxX, PxY);
	const float Hx1 = AlturaWorld(PxX + 1, PxY);
	const float Hx0 = AlturaWorld(PxX - 1, PxY);
	const float Hy1 = AlturaWorld(PxX, PxY + 1);
	const float Hy0 = AlturaWorld(PxX, PxY - 1);
	const FVector Dx((Hx1 - Hx0) * 0.5f, 0.f, (float)EscalaXY);
	const FVector Dy(0.f, (Hy1 - Hy0) * 0.5f, (float)EscalaXY);
	return FVector::CrossProduct(Dy, Dx).GetSafeNormal();
}

float ATerrenoGenerado::AlturaEnMundo(float X, float Y) const
{
	if (AlturasRAW.Num() == 0) return LocZ;

	const double MitadM = (ResolucionRAW - 1) * EscalaXY * 0.5;
	const FVector Centro(191800.0, 857000.0, 0.0);

	const double U = (X - (Centro.X - MitadM)) / EscalaXY;
	const double V = (Y - (Centro.Y - MitadM)) / EscalaXY;

	const int32 Ix = FMath::Clamp((int32)U, 0, ResolucionRAW - 2);
	const int32 Iy = FMath::Clamp((int32)V, 0, ResolucionRAW - 2);
	const float Fx = (float)(U - Ix);
	const float Fy = (float)(V - Iy);

	const float H00 = AlturaWorld(Ix, Iy);
	const float H10 = AlturaWorld(Ix + 1, Iy);
	const float H01 = AlturaWorld(Ix, Iy + 1);
	const float H11 = AlturaWorld(Ix + 1, Iy + 1);

	return FMath::Lerp(FMath::Lerp(H00, H10, Fx), FMath::Lerp(H01, H11, Fx), Fy);
}

FVector ATerrenoGenerado::NormalEnMundo(float X, float Y) const
{
	if (AlturasRAW.Num() == 0) return FVector::UpVector;

	const double MitadM = (ResolucionRAW - 1) * EscalaXY * 0.5;
	const FVector Centro(191800.0, 857000.0, 0.0);

	const double U = (X - (Centro.X - MitadM)) / EscalaXY;
	const double V = (Y - (Centro.Y - MitadM)) / EscalaXY;

	const int32 Ix = FMath::Clamp((int32)U, 0, ResolucionRAW - 1);
	const int32 Iy = FMath::Clamp((int32)V, 0, ResolucionRAW - 1);
	return CalcNormal(Ix, Iy);
}

void ATerrenoGenerado::GenerarDesdeRAW(const FString& RutaR16, int32 Res,
	double EscXY, double EscZ, double LZ, FVector CentroXY)
{
	if (AlturasRAW.Num() == 0)
	{
		CargarRAW(RutaR16);
		if (AlturasRAW.Num() == 0) return;
	}

	const int32 PixelesPorChunk = SizeChunkPx;
	NumChunksX = (Res - 1) / PixelesPorChunk;
	NumChunksY = (Res - 1) / PixelesPorChunk;
	if ((Res - 1) % PixelesPorChunk != 0) { NumChunksX++; NumChunksY++; }

	const double MitadM = (Res - 1) * EscXY * 0.5;
	const double OriginX = CentroXY.X - MitadM;
	const double OriginY = CentroXY.Y - MitadM;

	UE_LOG(LogTemp, Log, TEXT("[Terreno] Generando %d×%d chunks (%d px/chunk)"),
		NumChunksX, NumChunksY, PixelesPorChunk);

	ChunksInfo.SetNum(NumChunksX * NumChunksY);
	for (int32 Cy = 0; Cy < NumChunksY; Cy++)
	{
		for (int32 Cx = 0; Cx < NumChunksX; Cx++)
		{
			FInfoChunk& Info = ChunksInfo[Cy * NumChunksX + Cx];
			Info.ChunkX = Cx;
			Info.ChunkY = Cy;
			Info.OffsetPxX = Cx * PixelesPorChunk;
			Info.OffsetPxY = Cy * PixelesPorChunk;
			Info.AnchoPx = FMath::Min(PixelesPorChunk + 1, Res - Info.OffsetPxX);
			const int32 AltoPx = FMath::Min(PixelesPorChunk + 1, Res - Info.OffsetPxY);

			const double CentroLocalX = OriginX + (Info.OffsetPxX + (Info.AnchoPx - 1) * 0.5) * EscXY;
			const double CentroLocalY = OriginY + (Info.OffsetPxY + (AltoPx - 1) * 0.5) * EscXY;
			Info.CentroLocal = FVector(CentroLocalX, CentroLocalY, 0.0);
		}
	}

	for (const FInfoChunk& Info : ChunksInfo)
	{
		GenerarChunk(Info);
	}

	UE_LOG(LogTemp, Log, TEXT("[Terreno] Generado: %d chunks, origen (%.0f, %.0f)"),
		ChunksInfo.Num(), OriginX, OriginY);
}

void ATerrenoGenerado::GenerarChunk(const FInfoChunk& Info)
{
	const int32 Ancho = Info.AnchoPx;
	const int32 Alto = FMath::Min(SizeChunkPx + 1, ResolucionRAW - Info.OffsetPxY);
	const int32 NumVerts = Ancho * Alto;
	const int32 NumTris = (Ancho - 1) * (Alto - 1) * 6;

	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector2D> UV0;
	TArray<FColor> Colors;

	Vertices.Reserve(NumVerts);
	Triangles.Reserve(NumTris);
	Normals.Reserve(NumVerts);
	UV0.Reserve(NumVerts);
	Colors.Reserve(NumVerts);

	const double MitadM = (ResolucionRAW - 1) * EscalaXY * 0.5;
	const FVector Centro(191800.0, 857000.0, 0.0);
	const double OriginX = Centro.X - MitadM;
	const double OriginY = Centro.Y - MitadM;

	for (int32 Py = 0; Py < Alto; Py++)
	{
		for (int32 Px = 0; Px < Ancho; Px++)
		{
			const int32 Gx = Info.OffsetPxX + Px;
			const int32 Gy = Info.OffsetPxY + Py;
			const float Z = AlturaWorld(Gx, Gy);
			const FVector Pos(OriginX + Gx * EscalaXY, OriginY + Gy * EscalaXY, Z);
			Vertices.Add(Pos);
			Normals.Add(CalcNormal(Gx, Gy));
			UV0.Add(FVector2D((float)Px / (Ancho - 1), (float)Py / (Alto - 1)));

			const float N = (float)(LeerAltura(Gx, Gy) - 32000) / 32768.0f;
			const uint8 C = (uint8)FMath::Clamp(N * 128.f + 127.f, 0.f, 255.f);
			Colors.Add(FColor(C, C, C, 255));
		}
	}

	for (int32 Py = 0; Py < Alto - 1; Py++)
	{
		for (int32 Px = 0; Px < Ancho - 1; Px++)
		{
			const int32 I = Py * Ancho + Px;
			Triangles.Add(I);
			Triangles.Add(I + Ancho);
			Triangles.Add(I + 1);
			Triangles.Add(I + 1);
			Triangles.Add(I + Ancho);
			Triangles.Add(I + Ancho + 1);
		}
	}

	UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(this,
		FName(*FString::Printf(TEXT("Chunk_%d_%d"), Info.ChunkX, Info.ChunkY)));
	PMC->SetupAttachment(RootComponent);
	PMC->SetMobility(EComponentMobility::Static);
	PMC->bUseAsyncCooking = true;
	PMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
	PMC->SetCollisionObjectType(ECC_WorldStatic);
	PMC->SetCollisionResponseToAllChannels(ECR_Block);

	TArray<FLinearColor> LinearColors;
	LinearColors.Reserve(Colors.Num());
	for (const FColor& C : Colors) LinearColors.Add(FLinearColor(C));

	PMC->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, LinearColors, TArray<FProcMeshTangent>(), true);
	PMC->SetCastShadow(true);
	PMC->SetGenerateOverlapEvents(false);

	static UMaterialInterface* MatDefault = nullptr;
	if (!MatDefault)
		MatDefault = LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	if (MatDefault) PMC->SetMaterial(0, MatDefault);
}
