#include "TerrenoGenerado.h"
#include "ProceduralMeshComponent.h"
#include "KismetProceduralMeshLibrary.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
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
#if WITH_EDITOR
#include "MaterialEditingLibrary.h"
#include "EditorAssetLibrary.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#endif

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
			GenerarDesdeRAW(Ruta, ResolucionRAW, EscalaXY, EscalaZ, LocZ, CentroMundo());
		}
	}
}

void ATerrenoGenerado::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	ActualizarLODs();
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

	using ML = UMaterialEditingLibrary;

	// ── Vertex Color ────────────────────────────────────────────────────
	UMaterialExpressionVertexColor* VC = Cast<UMaterialExpressionVertexColor>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionVertexColor::StaticClass(), -800, 0));

	// ── Colores base ────────────────────────────────────────────────────
	UMaterialExpressionConstant4Vector* CWater = Cast<UMaterialExpressionConstant4Vector>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant4Vector::StaticClass(), -800, -280));
	CWater->Constant = FLinearColor(0.05f, 0.12f, 0.18f, 1.f);

	UMaterialExpressionConstant4Vector* CGrass = Cast<UMaterialExpressionConstant4Vector>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant4Vector::StaticClass(), -800, -140));
	CGrass->Constant = FLinearColor(0.15f, 0.32f, 0.10f, 1.f);

	UMaterialExpressionConstant4Vector* CRock = Cast<UMaterialExpressionConstant4Vector>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant4Vector::StaticClass(), -800, 80));
	CRock->Constant = FLinearColor(0.28f, 0.24f, 0.20f, 1.f);

	UMaterialExpressionConstant4Vector* CSnow = Cast<UMaterialExpressionConstant4Vector>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant4Vector::StaticClass(), -800, 280));
	CSnow->Constant = FLinearColor(0.72f, 0.70f, 0.68f, 1.f);

	// ── Blend por altura (VC.R): Water → Grass ─────────────────────────
	UMaterialExpressionLinearInterpolate* LerpH = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), -200, -200));
	ML::ConnectMaterialExpressions(CWater, TEXT(""), LerpH, TEXT("A"));
	ML::ConnectMaterialExpressions(CGrass, TEXT(""), LerpH, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("R"), LerpH, TEXT("Alpha"));

	// ── Blend por pendiente (VC.G): grass → rock ───────────────────────
	UMaterialExpressionLinearInterpolate* LerpS = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), 200, 80));
	ML::ConnectMaterialExpressions(LerpH, TEXT(""), LerpS, TEXT("A"));
	ML::ConnectMaterialExpressions(CRock, TEXT(""), LerpS, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("G"), LerpS, TEXT("Alpha"));

	// ── Blend por altitud (VC.R > 0.6 → snow) ──────────────────────────
	// Usamos LerpS como base y blend con snow por encima de 0.6
	UMaterialExpressionLinearInterpolate* LerpA = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), 600, 200));
	ML::ConnectMaterialExpressions(LerpS, TEXT(""), LerpA, TEXT("A"));
	ML::ConnectMaterialExpressions(CSnow, TEXT(""), LerpA, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("R"), LerpA, TEXT("Alpha"));

	// ── Roughness por pendiente (plano = 0.8, pendiente = 0.6) ──────────
	UMaterialExpressionConstant* RFlat = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -400, 460));
	RFlat->R = 0.8f;
	UMaterialExpressionConstant* RSteep = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -400, 540));
	RSteep->R = 0.6f;
	UMaterialExpressionLinearInterpolate* LerpR = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), -100, 500));
	ML::ConnectMaterialExpressions(RFlat, TEXT(""), LerpR, TEXT("A"));
	ML::ConnectMaterialExpressions(RSteep, TEXT(""), LerpR, TEXT("B"));
	ML::ConnectMaterialExpressions(VC, TEXT("G"), LerpR, TEXT("Alpha"));

	// ── Connect outputs ─────────────────────────────────────────────────
	ML::ConnectMaterialProperty(LerpA, TEXT(""), MP_BaseColor);
	ML::ConnectMaterialProperty(LerpR, TEXT(""), MP_Roughness);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);

	UE_LOG(LogTemp, Log, TEXT("[Terreno] Material M_TerrenoAlsasua creado en %s"), *Ruta);
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
	for (FInfoChunk& Info : ChunksInfo)
	{
		GenerarChunk(Info, 0, LOD0Step);
		GenerarChunk(Info, 1, LOD1Step);
		GenerarChunk(Info, 2, LOD2Step);
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
