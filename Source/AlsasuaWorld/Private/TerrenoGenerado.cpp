#include "TerrenoGenerado.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionConstant4Vector.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionOneMinus.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionDivide.h"
#include "Materials/MaterialExpressionTextureSample.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionScalarParameter.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionAppendVector.h"
#if WITH_EDITOR
#include "MaterialEditingLibrary.h"
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#endif

static constexpr int32 CHUNKS_POR_FRAME = 4;

ATerrenoGenerado::ATerrenoGenerado()
{
	PrimaryActorTick.bCanEverTick = true;
	PrimaryActorTick.TickInterval = 0.1f;
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
			if (bUsarLidar) CargarLidar();
			GenerarDesdeRAW(Ruta, ResolucionRAW, EscalaXY, EscalaZ, LocZ, CentroMundo());
		}
	}
}

void ATerrenoGenerado::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	if (ChunkIndexProgreso < ChunksInfo.Num())
	{
		GenerarSiguienteChunk();
	}
	else
	{
		ActualizarLODs();
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
		UE_LOG(LogTemp, Error, TEXT("[Terreno] Tamano %d != esperado %d (res %d)"), Bytes.Num(), Esperado, ResolucionRAW);
		return;
	}
	AlturasRAW.SetNumUninitialized(ResolucionRAW * ResolucionRAW);
	FMemory::Memcpy(AlturasRAW.GetData(), Bytes.GetData(), Esperado);
	UE_LOG(LogTemp, Log, TEXT("[Terreno] RAW cargado: %d bytes, %dx%d"), Bytes.Num(), ResolucionRAW, ResolucionRAW);
}

uint16 ATerrenoGenerado::LeerAltura(int32 PxX, int32 PxY) const
{
	PxX = FMath::Clamp(PxX, 0, ResolucionRAW - 1);
	PxY = FMath::Clamp(PxY, 0, ResolucionRAW - 1);
	return AlturasRAW[PxY * ResolucionRAW + PxX];
}

float ATerrenoGenerado::AlturaWorld(int32 PxX, int32 PxY) const
{
	const float AltoDTM = (float)LeerAltura(PxX, PxY) / 64.0f * (float)EscalaZ + (float)LocZ;

	if (bLidarFusionado || !bLidarCargado) return AltoDTM;

	const double WX = OriginWorld.X + PxX * EscalaXY;
	const double WY = OriginWorld.Y + PxY * EscalaXY;

	double DistBordeCm = 0.0;
	if (!DentroLidar(WX, WY, DistBordeCm)) return AltoDTM;

	const float AltoLidarCm = SampleLidarBicubic(WX, WY);

	// Blend gaussiano: 0 en el borde del área LIDAR, 1 en el interior.
	const double BlendCm = LidarBlendMetros * 100.0;
	const double T = (BlendCm <= 0.0) ? 1.0 : (1.0 - FMath::Exp(-3.0 * FMath::Square(FMath::Clamp(DistBordeCm / BlendCm, 0.0, 1.0))));

	return FMath::Lerp(AltoDTM, AltoLidarCm, (float)T);
}

// ─────────────────────────────────────────────────────────────────────────────
//  LIDAR 0.5m real (Assets/AlsasuaData/lidar_dtm_05m.raw del proyecto original)
// ─────────────────────────────────────────────────────────────────────────────

void ATerrenoGenerado::CargarLidar()
{
	const FString RutaMeta = FPaths::Combine(FPaths::ProjectContentDir(), RutaLidarMeta);
	FString MetaTexto;
	if (!FFileHelper::LoadFileToString(MetaTexto, *RutaMeta))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Terreno][LIDAR] No se encontro %s — se usa solo DTM ancho"), *RutaMeta);
		return;
	}

	TSharedPtr<FJsonObject> Meta;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(MetaTexto);
	if (!FJsonSerializer::Deserialize(Reader, Meta) || !Meta.IsValid())
	{
		UE_LOG(LogTemp, Error, TEXT("[Terreno][LIDAR] JSON invalido en %s"), *RutaMeta);
		return;
	}

	LidarRes     = Meta->GetIntegerField(TEXT("heightmapResolution"));
	LidarWidthM  = Meta->GetNumberField(TEXT("terrainWidth"));
	LidarLengthM = Meta->HasField(TEXT("terrainLength")) ? Meta->GetNumberField(TEXT("terrainLength")) : LidarWidthM;
	LidarZMinM   = Meta->GetNumberField(TEXT("z_min"));
	LidarZMaxM   = Meta->GetNumberField(TEXT("z_max"));

	const FString RutaRawLidar = FPaths::Combine(FPaths::ProjectContentDir(), RutaLidarRAW);
	TArray<uint8> Bytes;
	if (!FFileHelper::LoadFileToArray(Bytes, *RutaRawLidar))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Terreno][LIDAR] No se pudo leer %s"), *RutaRawLidar);
		return;
	}

	const int32 Esperado = LidarRes * LidarRes * 2;
	if (Bytes.Num() != Esperado)
	{
		UE_LOG(LogTemp, Error, TEXT("[Terreno][LIDAR] Tamano %d != esperado %d (res %d)"), Bytes.Num(), Esperado, LidarRes);
		return;
	}

	LidarRaw.SetNumUninitialized(LidarRes * LidarRes);
	FMemory::Memcpy(LidarRaw.GetData(), Bytes.GetData(), Esperado);
	bLidarCargado = true;

	UE_LOG(LogTemp, Log, TEXT("[Terreno][LIDAR] Cargado: %dx%d @ 0.5m, area %.0fx%.0fm, Z %.1f-%.1fm"),
		LidarRes, LidarRes, LidarWidthM, LidarLengthM, LidarZMinM, LidarZMaxM);
}

bool ATerrenoGenerado::DentroLidar(double WorldXcm, double WorldYcm, double& OutDistBordeCm) const
{
	if (!bLidarCargado) return false;

	const FVector Centro = CentroMundo();
	const double HalfXcm = LidarWidthM * 100.0 * 0.5;
	const double HalfYcm = LidarLengthM * 100.0 * 0.5;
	const double Dx = WorldXcm - Centro.X;
	const double Dy = WorldYcm - Centro.Y;

	if (FMath::Abs(Dx) > HalfXcm || FMath::Abs(Dy) > HalfYcm) return false;

	OutDistBordeCm = FMath::Min(HalfXcm - FMath::Abs(Dx), HalfYcm - FMath::Abs(Dy));
	return true;
}

float ATerrenoGenerado::SampleLidarBicubic(double WorldXcm, double WorldYcm) const
{
	const FVector Centro = CentroMundo();
	const double WidthCm  = LidarWidthM * 100.0;
	const double LengthCm = LidarLengthM * 100.0;
	const double OriginXcm = Centro.X - WidthCm * 0.5;
	const double OriginYcm = Centro.Y - LengthCm * 0.5;

	const double Fx = (WorldXcm - OriginXcm) / WidthCm;
	const double Fy = (WorldYcm - OriginYcm) / LengthCm;

	const double Px = FMath::Clamp(Fx * (LidarRes - 1), 0.5, LidarRes - 1.5);
	const double Py = FMath::Clamp(Fy * (LidarRes - 1), 0.5, LidarRes - 1.5);
	const int32 Ix = (int32)Px;
	const int32 Iy = (int32)Py;
	const float Tx = (float)(Px - Ix);
	const float Ty = (float)(Py - Iy);

	auto GetH = [this](int32 R, int32 C) -> float
	{
		R = FMath::Clamp(R, 0, LidarRes - 1);
		C = FMath::Clamp(C, 0, LidarRes - 1);
		const uint16 Raw = LidarRaw[R * LidarRes + C];
		return (float)Raw / 65535.f * (float)(LidarZMaxM - LidarZMinM) + (float)LidarZMinM; // metros reales
	};

	auto CatmullRom = [](float T, float P0, float P1, float P2, float P3) -> float
	{
		return 0.5f * (
			(2.f * P1) +
			(-P0 + P2) * T +
			(2.f * P0 - 5.f * P1 + 4.f * P2 - P3) * T * T +
			(-P0 + 3.f * P1 - 3.f * P2 + P3) * T * T * T);
	};

	const float R0 = CatmullRom(Tx, GetH(Iy - 1, Ix - 1), GetH(Iy - 1, Ix), GetH(Iy - 1, Ix + 1), GetH(Iy - 1, Ix + 2));
	const float R1 = CatmullRom(Tx, GetH(Iy,     Ix - 1), GetH(Iy,     Ix), GetH(Iy,     Ix + 1), GetH(Iy,     Ix + 2));
	const float R2 = CatmullRom(Tx, GetH(Iy + 1, Ix - 1), GetH(Iy + 1, Ix), GetH(Iy + 1, Ix + 1), GetH(Iy + 1, Ix + 2));
	const float R3 = CatmullRom(Tx, GetH(Iy + 2, Ix - 1), GetH(Iy + 2, Ix), GetH(Iy + 2, Ix + 1), GetH(Iy + 2, Ix + 2));

	const float AltMReal = CatmullRom(Ty, R0, R1, R2, R3);
	return AltMReal * 100.f; // metros → cm (mismo espacio absoluto que AltoDTM)
}

// ─────────────────────────────────────────────────────────────────────────────
//  Validación RMSE contra lidar_ground.xyz (puntos de control reales)
// ─────────────────────────────────────────────────────────────────────────────

void ATerrenoGenerado::ValidarTerrenoRMSE()
{
	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaLidarGround);
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{
		UE_LOG(LogTemp, Error, TEXT("[Validacion] No se pudo leer %s"), *Ruta);
		return;
	}

	TArray<FString> Lineas;
	Texto.ParseIntoArrayLines(Lineas);

	double SumSq = 0.0, SumAbs = 0.0;
	float MaxErr = 0.f;
	int32 Contados = 0;

	for (const FString& Linea : Lineas)
	{
		TArray<FString> Tok;
		Linea.ParseIntoArrayWS(Tok);
		if (Tok.Num() < 3) continue;

		const double X = FCString::Atod(*Tok[0]);
		const double Y = FCString::Atod(*Tok[1]); // altura real (m)
		const double Z = FCString::Atod(*Tok[2]);

		// lidar_ground.xyz está en coords absolutas del mundo (m): X este, Z norte, Y altura.
		const double WX = X * 100.0;
		const double WY = Z * 100.0;
		const float AltoTerreno = AlturaEnMundo((float)WX, (float)WY); // cm
		const float AltoRealCm = (float)(Y * 100.0);

		const float Delta = (AltoTerreno - AltoRealCm) / 100.f; // error en metros
		SumSq += Delta * Delta;
		SumAbs += FMath::Abs(Delta);
		MaxErr = FMath::Max(MaxErr, FMath::Abs(Delta));
		Contados++;
	}

	if (Contados == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Validacion] Sin puntos validos en %s"), *Ruta);
		return;
	}

	const double RMSE = FMath::Sqrt(SumSq / Contados);
	const double MAE = SumAbs / Contados;
	const FString Estado = RMSE < 0.3 ? TEXT("OK") : (RMSE < 0.6 ? TEXT("ADVERTENCIA") : TEXT("CRITICO"));

	UE_LOG(LogTemp, Log, TEXT("[Validacion] RMSE=%.3fm MAE=%.3fm MAX=%.3fm (%d puntos) — Estado: %s"),
		RMSE, MAE, MaxErr, Contados, *Estado);
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

FVector4 ATerrenoGenerado::CalcTangent(int32 PxX, int32 PxY) const
{
	const FVector N = CalcNormal(PxX, PxY);
	const float Hx1 = AlturaWorld(PxX + 1, PxY);
	const float Hx0 = AlturaWorld(PxX - 1, PxY);
	FVector Tangent(Hx1 - Hx0, 0.f, 0.f);
	Tangent = Tangent.GetSafeNormal();
	const FVector BiNormal = FVector::CrossProduct(N, Tangent).GetSafeNormal();
	const float BiTangentSign = (FVector::CrossProduct(N, Tangent).Dot(BiNormal) < 0.f) ? -1.f : 1.f;
	return FVector4(Tangent.X, Tangent.Y, Tangent.Z, BiTangentSign);
}

FLinearColor ATerrenoGenerado::CalcVertexColor(int32 PxX, int32 PxY) const
{
	const float H = AlturaWorld(PxX, PxY);
	const float HNorm = (float)FMath::Clamp((H - (float)HeightMinCm) / (float)(HeightMaxCm - HeightMinCm), 0.0, 1.0);

	const float Hx1 = AlturaWorld(PxX + 1, PxY);
	const float Hx0 = AlturaWorld(PxX - 1, PxY);
	const float Hy1 = AlturaWorld(PxX, PxY + 1);
	const float Hy0 = AlturaWorld(PxX, PxY - 1);
	const float Dx = (Hx1 - Hx0) * 0.5f;
	const float Dy = (Hy1 - Hy0) * 0.5f;
	const float Slope = FMath::Clamp(FMath::Sqrt(Dx * Dx + Dy * Dy) / (float)EscalaXY, 0.f, 1.f);

	const FVector Centro = CentroMundo();
	const double MitadM = MitadMundo();
	const double WX = (double)(OriginWorld.X + PxX * EscalaXY);
	const double WY = (double)(OriginWorld.Y + PxY * EscalaXY);
	const double DistFromCenter = FMath::Sqrt(
		FMath::Square(WX - Centro.X) + FMath::Square(WY - Centro.Y));
	const float DistNorm = (float)FMath::Clamp(DistFromCenter / MitadM, 0.0, 1.0);

	return FLinearColor(HNorm, Slope, DistNorm, 1.f);
}

float ATerrenoGenerado::AlturaEnMundo(float X, float Y) const
{
	if (AlturasRAW.Num() == 0) return (float)LocZ;

	const double MitadM = MitadMundo();
	const FVector Centro = CentroMundo();

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

	const double MitadM = MitadMundo();
	const FVector Centro = CentroMundo();

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

	OriginWorld = FVector(CentroXY.X - MitadMundo(), CentroXY.Y - MitadMundo(), 0.0);

	// Funde el LIDAR en AlturasRAW una sola vez. BuildMeshData llama AlturaWorld
	// ~18 veces por vértice; sin este cache serían 18 muestras bicúbicas+exp por vértice.
	if (bUsarLidar && bLidarCargado && !bLidarFusionado)
	{
		const uint64 T0 = FPlatformTime::Cycles64();
		TArray<uint16> Fusion;
		Fusion.SetNumUninitialized(ResolucionRAW * ResolucionRAW);
		for (int32 Py = 0; Py < ResolucionRAW; Py++)
		{
			for (int32 Px = 0; Px < ResolucionRAW; Px++)
			{
				const float AltoCm = AlturaWorld(Px, Py);
				const int32 Raw = FMath::RoundToInt((AltoCm - (float)LocZ) / (float)EscalaZ * 64.0);
				Fusion[Py * ResolucionRAW + Px] = (uint16)FMath::Clamp(Raw, 0, 65535);
			}
		}
		AlturasRAW = MoveTemp(Fusion);
		bLidarFusionado = true;
		const double Ms = FPlatformTime::ToMilliseconds64(FPlatformTime::Cycles64() - T0);
		UE_LOG(LogTemp, Log, TEXT("[Terreno][LIDAR] Blend precomputado en %.0f ms"), Ms);
	}

	const int32 PixelesPorChunk = SizeChunkPx;
	NumChunksX = (Res - 1) / PixelesPorChunk;
	NumChunksY = (Res - 1) / PixelesPorChunk;
	if ((Res - 1) % PixelesPorChunk != 0) { NumChunksX++; NumChunksY++; }

	UE_LOG(LogTemp, Log, TEXT("[Terreno] Generando %dx%d chunks (%d px/chunk), LOD0=%d, LOD1=%d, LOD2=%d"),
		NumChunksX, NumChunksY, PixelesPorChunk, LOD0Step, LOD1Step, LOD2Step);

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
			Info.AltoPx = FMath::Min(PixelesPorChunk + 1, Res - Info.OffsetPxY);

			const double CentroLocalX = OriginWorld.X + (Info.OffsetPxX + (Info.AnchoPx - 1) * 0.5) * EscXY;
			const double CentroLocalY = OriginWorld.Y + (Info.OffsetPxY + (Info.AltoPx - 1) * 0.5) * EscXY;
			Info.CentroWorld = FVector(CentroLocalX, CentroLocalY, 0.0);
		}
	}

	GenerarTodosChunks();

	UE_LOG(LogTemp, Log, TEXT("[Terreno] Generado: %d chunks, origen (%.0f, %.0f)"),
		ChunksInfo.Num(), OriginWorld.X, OriginWorld.Y);
}

void ATerrenoGenerado::BuildMeshData(const FInfoChunk& Info, int32 Step,
	TArray<FVector>& OutVerts, TArray<int32>& OutTris,
	TArray<FVector>& OutNormals, TArray<FVector4>& OutTangents,
	TArray<FVector2D>& OutUV0, TArray<FLinearColor>& OutColors) const
{
	const int32 Ancho = Info.AnchoPx;
	const int32 Alto = Info.AltoPx;
	const int32 NumVertsX = (Ancho - 1) / Step + 1;
	const int32 NumVertsY = (Alto - 1) / Step + 1;
	const int32 NumVerts = NumVertsX * NumVertsY;
	const int32 NumTris = (NumVertsX - 1) * (NumVertsY - 1) * 6;

	OutVerts.Reserve(NumVerts);
	OutTris.Reserve(NumTris);
	OutNormals.Reserve(NumVerts);
	OutTangents.Reserve(NumVerts);
	OutUV0.Reserve(NumVerts);
	OutColors.Reserve(NumVerts);

	for (int32 Vy = 0; Vy < NumVertsY; Vy++)
	{
		for (int32 Vx = 0; Vx < NumVertsX; Vx++)
		{
			const int32 Px = Info.OffsetPxX + Vx * Step;
			const int32 Py = Info.OffsetPxY + Vy * Step;
			const float Z = AlturaWorld(Px, Py);
			const FVector Pos(OriginWorld.X + Px * EscalaXY, OriginWorld.Y + Py * EscalaXY, Z);
			OutVerts.Add(Pos);
			OutNormals.Add(CalcNormal(Px, Py));
			OutTangents.Add(CalcTangent(Px, Py));
			OutUV0.Add(FVector2D((float)Vx / FMath::Max(NumVertsX - 1, 1), (float)Vy / FMath::Max(NumVertsY - 1, 1)));
			OutColors.Add(CalcVertexColor(Px, Py));
		}
	}

	for (int32 Vy = 0; Vy < NumVertsY - 1; Vy++)
	{
		for (int32 Vx = 0; Vx < NumVertsX - 1; Vx++)
		{
			const int32 I = Vy * NumVertsX + Vx;
			OutTris.Add(I);
			OutTris.Add(I + NumVertsX);
			OutTris.Add(I + 1);
			OutTris.Add(I + 1);
			OutTris.Add(I + NumVertsX);
			OutTris.Add(I + NumVertsX + 1);
		}
	}
}

UMaterialInterface* ATerrenoGenerado::CrearMaterialTerreno()
{
#if WITH_EDITOR
	// -game / standalone sin editor: UEditorAssetLibrary no es utilizable.
	if (!GEditor)
	{
		if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_TerrenoAlsasua.M_TerrenoAlsasua")))
		{
			return Mat;
		}
		if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Terreno_Orto.M_Terreno_Orto")))
		{
			return Mat;
		}
		return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
	}

	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre  = TEXT("M_TerrenoAlsasua");
	const FString Ruta    = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
	{
		UMaterialInterface* Existente = LoadObject<UMaterialInterface>(nullptr, *(Ruta + TEXT(".") + Nombre));
		if (Existente) return Existente;
	}

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UObject* Obj = AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>());
	UMaterial* Mat = Cast<UMaterial>(Obj);
	if (!Mat)
	{
		UE_LOG(LogTemp, Error, TEXT("[Terreno] No se pudo crear M_TerrenoAlsasua"));
		return nullptr;
	}

	Mat->BlendMode = EBlendMode::BLEND_Opaque;
	Mat->SetShadingModel(EMaterialShadingModel::MSM_DefaultLit);
	Mat->TwoSided = false;

	using ML = UMaterialEditingLibrary;

	// ── World Position for tiling ──────────────────────────────────────
	UMaterialExpressionWorldPosition* WP = Cast<UMaterialExpressionWorldPosition>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionWorldPosition::StaticClass(), -1200, 400));

	UMaterialExpressionScalarParameter* TileScale = Cast<UMaterialExpressionScalarParameter>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionScalarParameter::StaticClass(), -1200, 500));
	TileScale->ParameterName = TEXT("TilingScale");
	TileScale->DefaultValue = 1.0f / FMath::Max(TextureTilingCm, 1.0f);

	UMaterialExpressionMultiply* TileXY = Cast<UMaterialExpressionMultiply>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), -900, 450));
	ML::ConnectMaterialExpressions(WP, TEXT(""), TileXY, TEXT("A"));
	ML::ConnectMaterialExpressions(TileScale, TEXT(""), TileXY, TEXT("B"));

	// ── Vertex Color ────────────────────────────────────────────────────
	UMaterialExpressionVertexColor* VC = Cast<UMaterialExpressionVertexColor>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionVertexColor::StaticClass(), -800, 0));

	// ── TEXTURE SAMPLES ─────────────────────────────────────────────────

	auto SafeLoad = [](TSoftObjectPtr<UTexture>& Slot, const TCHAR* Fallback) -> UTexture*
	{
		if (Slot.IsValid()) return Slot.Get();
		if (Slot.IsPending()) { UTexture* T = Slot.LoadSynchronous(); if (T) return T; }
		return LoadObject<UTexture>(nullptr, Fallback);
	};

	// ── Ortofoto satelital real, draped sobre todo el terreno ───────────
	const double SatExtentCm = (ResolucionRAW - 1) * EscalaXY;
	const double SatOriginX = OriginWorld.X;
	const double SatOriginY = OriginWorld.Y;

	UMaterialExpressionComponentMask* WPMaskX = Cast<UMaterialExpressionComponentMask>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionComponentMask::StaticClass(), -1200, 700));
	WPMaskX->R = 1; WPMaskX->G = 0; WPMaskX->B = 0; WPMaskX->A = 0;
	ML::ConnectMaterialExpressions(WP, TEXT(""), WPMaskX, TEXT(""));

	UMaterialExpressionComponentMask* WPMaskY = Cast<UMaterialExpressionComponentMask>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionComponentMask::StaticClass(), -1200, 760));
	WPMaskY->R = 0; WPMaskY->G = 1; WPMaskY->B = 0; WPMaskY->A = 0;
	ML::ConnectMaterialExpressions(WP, TEXT(""), WPMaskY, TEXT(""));

	UMaterialExpressionConstant* SatOriginXExpr = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -1050, 700));
	SatOriginXExpr->R = (float)SatOriginX;
	UMaterialExpressionConstant* SatOriginYExpr = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -1050, 760));
	SatOriginYExpr->R = (float)SatOriginY;

	UMaterialExpressionSubtract* SubX = Cast<UMaterialExpressionSubtract>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionSubtract::StaticClass(), -900, 700));
	ML::ConnectMaterialExpressions(WPMaskX, TEXT(""), SubX, TEXT("A"));
	ML::ConnectMaterialExpressions(SatOriginXExpr, TEXT(""), SubX, TEXT("B"));

	UMaterialExpressionSubtract* SubY = Cast<UMaterialExpressionSubtract>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionSubtract::StaticClass(), -900, 760));
	ML::ConnectMaterialExpressions(WPMaskY, TEXT(""), SubY, TEXT("A"));
	ML::ConnectMaterialExpressions(SatOriginYExpr, TEXT(""), SubY, TEXT("B"));

	UMaterialExpressionScalarParameter* InvExtent = Cast<UMaterialExpressionScalarParameter>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionScalarParameter::StaticClass(), -900, 820));
	InvExtent->ParameterName = TEXT("SatInvExtent");
	InvExtent->DefaultValue = 1.0f / FMath::Max((float)SatExtentCm, 1.0f);

	UMaterialExpressionMultiply* MulU = Cast<UMaterialExpressionMultiply>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), -750, 700));
	ML::ConnectMaterialExpressions(SubX, TEXT(""), MulU, TEXT("A"));
	ML::ConnectMaterialExpressions(InvExtent, TEXT(""), MulU, TEXT("B"));

	UMaterialExpressionMultiply* MulV = Cast<UMaterialExpressionMultiply>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), -750, 760));
	ML::ConnectMaterialExpressions(SubY, TEXT(""), MulV, TEXT("A"));
	ML::ConnectMaterialExpressions(InvExtent, TEXT(""), MulV, TEXT("B"));

	UMaterialExpressionAppendVector* SatUV = Cast<UMaterialExpressionAppendVector>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionAppendVector::StaticClass(), -600, 730));
	ML::ConnectMaterialExpressions(MulU, TEXT(""), SatUV, TEXT("A"));
	ML::ConnectMaterialExpressions(MulV, TEXT(""), SatUV, TEXT("B"));

	UTexture* SatTex = SafeLoad(SatelliteImage, TEXT("/Game/Terreno/T_Satelite_Alsasua.T_Satelite_Alsasua"));
	UMaterialExpressionTextureSample* SatSample = Cast<UMaterialExpressionTextureSample>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSample::StaticClass(), -400, 730));
	if (SatTex) { SatSample->Texture = SatTex; SatSample->SamplerType = SAMPLERTYPE_Color; }
	ML::ConnectMaterialExpressions(SatUV, TEXT(""), SatSample, TEXT("UVs"));

	UTexture* GrassTex = SafeLoad(GrassDiffuse,
		TEXT("/Game/AssetsImportados/TexturasUnity/GrassPBR_Color.GrassPBR_Color"));
	UMaterialExpressionTextureSample* GrassDiff = Cast<UMaterialExpressionTextureSample>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSample::StaticClass(), -800, -350));
	if (GrassTex) { GrassDiff->Texture = GrassTex; GrassDiff->SamplerType = SAMPLERTYPE_Color; }
	ML::ConnectMaterialExpressions(TileXY, TEXT(""), GrassDiff, TEXT("UVs"));

	UTexture* RockTex = SafeLoad(RockDiffuse,
		TEXT("/Game/AssetsImportados/Naturaleza/rocks/01/diffuse"));
	UMaterialExpressionTextureSample* RockDiff = Cast<UMaterialExpressionTextureSample>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSample::StaticClass(), -800, -100));
	if (RockTex) { RockDiff->Texture = RockTex; RockDiff->SamplerType = SAMPLERTYPE_Color; }
	ML::ConnectMaterialExpressions(TileXY, TEXT(""), RockDiff, TEXT("UVs"));

	UTexture* GroundTex = SafeLoad(GroundDiffuse,
		TEXT("/Game/AssetsImportados/TexturasUnity/GroundPBR_Color.GroundPBR_Color"));
	UMaterialExpressionTextureSample* GroundDiff = Cast<UMaterialExpressionTextureSample>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSample::StaticClass(), -800, 100));
	if (GroundTex) { GroundDiff->Texture = GroundTex; GroundDiff->SamplerType = SAMPLERTYPE_Color; }
	ML::ConnectMaterialExpressions(TileXY, TEXT(""), GroundDiff, TEXT("UVs"));

	UMaterialExpressionConstant4Vector* CSnow = Cast<UMaterialExpressionConstant4Vector>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant4Vector::StaticClass(), -800, 300));
	CSnow->Constant = FLinearColor(0.85f, 0.85f, 0.88f, 1.f);

	UMaterialExpressionConstant4Vector* CWater = Cast<UMaterialExpressionConstant4Vector>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant4Vector::StaticClass(), -800, -500));
	CWater->Constant = FLinearColor(0.05f, 0.12f, 0.18f, 1.f);

	// ── NORMAL MAPS ─────────────────────────────────────────────────────
	UTexture* GrassNormTex = SafeLoad(GrassNormal,
		TEXT("/Game/AssetsImportados/TexturasUnity/GrassPBR_Normal.GrassPBR_Normal"));
	UTexture* RockNormTex = SafeLoad(RockNormal,
		TEXT("/Game/AssetsImportados/Naturaleza/rocks/01/normal"));
	UTexture* GroundNormTex = SafeLoad(GroundNormal,
		TEXT("/Game/AssetsImportados/TexturasUnity/GroundPBR_Normal.GroundPBR_Normal"));

	UMaterialExpressionTextureSample* GrassNorm = Cast<UMaterialExpressionTextureSample>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSample::StaticClass(), -400, -350));
	if (GrassNormTex) { GrassNorm->Texture = GrassNormTex; GrassNorm->SamplerType = SAMPLERTYPE_LinearColor; }
	ML::ConnectMaterialExpressions(TileXY, TEXT(""), GrassNorm, TEXT("UVs"));

	UMaterialExpressionTextureSample* RockNorm = Cast<UMaterialExpressionTextureSample>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSample::StaticClass(), -400, -100));
	if (RockNormTex) { RockNorm->Texture = RockNormTex; RockNorm->SamplerType = SAMPLERTYPE_LinearColor; }
	ML::ConnectMaterialExpressions(TileXY, TEXT(""), RockNorm, TEXT("UVs"));

	UMaterialExpressionTextureSample* GroundNorm = Cast<UMaterialExpressionTextureSample>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSample::StaticClass(), -400, 100));
	if (GroundNormTex) { GroundNorm->Texture = GroundNormTex; GroundNorm->SamplerType = SAMPLERTYPE_LinearColor; }
	ML::ConnectMaterialExpressions(TileXY, TEXT(""), GroundNorm, TEXT("UVs"));

	// ── BLEND: Water → Grass (by VC.R height) ──────────────────────────
	UMaterialExpressionLinearInterpolate* LerpH = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), 0, -350));
	ML::ConnectMaterialExpressions(CWater, TEXT(""), LerpH, TEXT("A"));
	ML::ConnectMaterialExpressions(GrassDiff, TEXT(""), LerpH, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("R"), LerpH, TEXT("Alpha"));

	// ── BLEND: grass+water → Rock (by VC.G slope) ──────────────────────
	UMaterialExpressionLinearInterpolate* LerpS = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), 300, -100));
	ML::ConnectMaterialExpressions(LerpH, TEXT(""), LerpS, TEXT("A"));
	ML::ConnectMaterialExpressions(RockDiff, TEXT(""), LerpS, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("G"), LerpS, TEXT("Alpha"));

	// ── BLEND: base → Snow (by VC.R > 0.6 altitude) ────────────────────
	UMaterialExpressionLinearInterpolate* LerpA = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), 600, 0));
	ML::ConnectMaterialExpressions(LerpS, TEXT(""), LerpA, TEXT("A"));
	ML::ConnectMaterialExpressions(CSnow, TEXT(""), LerpA, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("R"), LerpA, TEXT("Alpha"));

	// ── NORMAL BLEND (same chain) ───────────────────────────────────────
	UMaterialExpressionLinearInterpolate* NormH = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), 0, 200));
	ML::ConnectMaterialExpressions(GrassNorm, TEXT(""), NormH, TEXT("A"));
	ML::ConnectMaterialExpressions(RockNorm, TEXT(""), NormH, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("G"), NormH, TEXT("Alpha"));

	// ── ROUGHNESS (grass=0.9, rock=0.7, snow=0.3) ──────────────────────
	UMaterialExpressionConstant* RG = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -400, 600));
	RG->R = 0.9f;
	UMaterialExpressionConstant* RR = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -400, 660));
	RR->R = 0.7f;
	UMaterialExpressionConstant* RS = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -400, 720));
	RS->R = 0.3f;

	UMaterialExpressionLinearInterpolate* LerpRG = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), -100, 620));
	ML::ConnectMaterialExpressions(RG, TEXT(""), LerpRG, TEXT("A"));
	ML::ConnectMaterialExpressions(RR, TEXT(""), LerpRG, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("G"), LerpRG, TEXT("Alpha"));

	UMaterialExpressionLinearInterpolate* LerpRRS = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), 200, 660));
	ML::ConnectMaterialExpressions(LerpRG, TEXT(""), LerpRRS, TEXT("A"));
	ML::ConnectMaterialExpressions(RS, TEXT(""), LerpRRS, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("R"), LerpRRS, TEXT("Alpha"));

	// ── Satelite drapeado, agua se mantiene en zonas bajas (VC.R) ──────
	UMaterialExpressionLinearInterpolate* LerpWaterSat = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), -200, -500));
	ML::ConnectMaterialExpressions(CWater, TEXT(""), LerpWaterSat, TEXT("A"));
	ML::ConnectMaterialExpressions(SatSample, TEXT(""), LerpWaterSat, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("R"), LerpWaterSat, TEXT("Alpha"));

	// ── Connect outputs ─────────────────────────────────────────────────
	ML::ConnectMaterialProperty(LerpWaterSat, TEXT(""), MP_BaseColor);
	ML::ConnectMaterialProperty(LerpRRS, TEXT(""), MP_Roughness);
	ML::ConnectMaterialProperty(NormH, TEXT(""), MP_Normal);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);

	UE_LOG(LogTemp, Log, TEXT("[Terreno] Material M_TerrenoAlsasua (PBR) creado en %s"), *Ruta);
	return Mat;
#else
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/WorldGridMaterial.WorldGridMaterial"));
#endif
}

void ATerrenoGenerado::GenerarChunk(FInfoChunk& Info, int32 LODLevel, int32 Step)
{
	TArray<FVector> Vertices;
	TArray<int32> Triangles;
	TArray<FVector> Normals;
	TArray<FVector4> Tangents;
	TArray<FVector2D> UV0;
	TArray<FLinearColor> Colors;

	BuildMeshData(Info, Step, Vertices, Triangles, Normals, Tangents, UV0, Colors);

	FString Name = FString::Printf(TEXT("Chunk_%d_%d_LOD%d"), Info.ChunkX, Info.ChunkY, LODLevel);
	UProceduralMeshComponent* PMC = NewObject<UProceduralMeshComponent>(this, FName(*Name));
	PMC->RegisterComponent();
	PMC->SetupAttachment(RootComponent);
	PMC->SetMobility(EComponentMobility::Static);
	PMC->bUseAsyncCooking = true;

	if (LODLevel == 0)
	{
		PMC->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
		PMC->SetCollisionObjectType(ECC_WorldStatic);
		PMC->SetCollisionResponseToAllChannels(ECR_Block);
	}
	else
	{
		PMC->SetCollisionEnabled(ECollisionEnabled::NoCollision);
	}

	TArray<FProcMeshTangent> ProcTangents;
	ProcTangents.Reserve(Tangents.Num());
	for (const FVector4& T : Tangents)
	{
		ProcTangents.Add(FProcMeshTangent(FVector(T.X, T.Y, T.Z), T.W < 0.f));
	}

	PMC->CreateMeshSection_LinearColor(0, Vertices, Triangles, Normals, UV0, Colors, ProcTangents, true);
	PMC->SetCastShadow(LODLevel == 0);
	PMC->SetGenerateOverlapEvents(false);

	if (LODLevel == 0)
	{
		UMaterialInterface* Mat = TerrainMaterial.LoadSynchronous();
		if (!Mat)
		{
			static UMaterialInterface* MatFallback = nullptr;
			if (!MatFallback)
				MatFallback = CrearMaterialTerreno();
			Mat = MatFallback;
		}
		if (Mat) PMC->SetMaterial(0, Mat);
	}
	else
	{
		static UMaterialInterface* MatLOD = nullptr;
		if (!MatLOD)
			MatLOD = CrearMaterialTerreno();
		if (MatLOD) PMC->SetMaterial(0, MatLOD);
	}

	PMC->SetMeshSectionVisible(0, true);

	if (LODLevel == 0)
	{
		Info.MeshLOD0 = PMC;
	}
	else if (LODLevel == 1)
	{
		PMC->SetMeshSectionVisible(0, false);
		Info.MeshLOD1 = PMC;
	}
	else
	{
		PMC->SetMeshSectionVisible(0, false);
		Info.MeshLOD2 = PMC;
	}
}

void ATerrenoGenerado::GenerarTodosChunks()
{
	ChunkIndexProgreso = 0;
	PrimaryActorTick.TickInterval = 0.0f;
	UE_LOG(LogTemp, Log, TEXT("[Terreno] %d chunks encolados (generando %d/frame)"), ChunksInfo.Num(), CHUNKS_POR_FRAME);
}

void ATerrenoGenerado::GenerarSiguienteChunk()
{
	const int32 Inicio = ChunkIndexProgreso;
	const int32 Fin = FMath::Min(ChunkIndexProgreso + CHUNKS_POR_FRAME, ChunksInfo.Num());
	for (int32 i = Inicio; i < Fin; i++)
	{
		FInfoChunk& Info = ChunksInfo[i];
		GenerarChunk(Info, 0, LOD0Step);
		GenerarChunk(Info, 1, LOD1Step);
		GenerarChunk(Info, 2, LOD2Step);
	}
	ChunkIndexProgreso = Fin;
	if (ChunkIndexProgreso >= ChunksInfo.Num())
	{
		PrimaryActorTick.TickInterval = 0.1f;
		UE_LOG(LogTemp, Log, TEXT("[Terreno] Todos los chunks generados (%d)"), ChunksInfo.Num());
	}
}

void ATerrenoGenerado::ActualizarLODs()
{
	APlayerController* PC = UGameplayStatics::GetPlayerController(this, 0);
	if (!PC) return;

	FVector CamLoc;
	FRotator CamRot;
	PC->GetPlayerViewPoint(CamLoc, CamRot);

	for (FInfoChunk& Info : ChunksInfo)
	{
		const double Dist = FVector::Dist(CamLoc, Info.CentroWorld);

		const bool bShowLOD0 = Dist < LOD1Distance;
		const bool bShowLOD1 = !bShowLOD0 && Dist < LOD2Distance;
		const bool bShowLOD2 = !bShowLOD0 && !bShowLOD1;

		if (Info.MeshLOD0) Info.MeshLOD0->SetMeshSectionVisible(0, bShowLOD0);
		if (Info.MeshLOD1) Info.MeshLOD1->SetMeshSectionVisible(0, bShowLOD1);
		if (Info.MeshLOD2) Info.MeshLOD2->SetMeshSectionVisible(0, bShowLOD2);
	}
}
