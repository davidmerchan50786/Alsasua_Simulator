#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrenoGenerado.generated.h"

class UProceduralMeshComponent;
class UMaterialInterface;

UCLASS()
class ALSASUAWORLD_API ATerrenoGenerado : public AActor
{
	GENERATED_BODY()

public:
	ATerrenoGenerado();

	void GenerarDesdeRAW(const FString& RutaR16, int32 Resolucion,
		double EscalaXY_cm, double EscalaZ, double LocZ_cm, FVector CentroXY_cm);

	float AlturaEnMundo(float X, float Y) const;
	FVector NormalEnMundo(float X, float Y) const;

	UPROPERTY(EditAnywhere, Category="Terreno") int32 SizeChunkPx = 128;
	UPROPERTY(EditAnywhere, Category="Terreno") int32 ResolucionRAW = 4033;
	UPROPERTY(EditAnywhere, Category="Terreno") double EscalaXY = 178.5714;
	UPROPERTY(EditAnywhere, Category="Terreno") double EscalaZ = 200.0;
	UPROPERTY(EditAnywhere, Category="Terreno") double LocZ = 49567.0;
	UPROPERTY(EditAnywhere, Category="Terreno") FString RutaRAW = TEXT("Terreno/alsasua_landscape_4033.r16");

	UPROPERTY(EditAnywhere, Category="Terreno|Material")
	TSoftObjectPtr<UMaterialInterface> TerrainMaterial;

	UPROPERTY(EditAnywhere, Category="Terreno|LOD")
	int32 LOD0Step = 1;

	UPROPERTY(EditAnywhere, Category="Terreno|LOD")
	int32 LOD1Step = 2;

	UPROPERTY(EditAnywhere, Category="Terreno|LOD")
	int32 LOD2Step = 4;

	UPROPERTY(EditAnywhere, Category="Terreno|LOD")
	double LOD1Distance = 80000.0;

	UPROPERTY(EditAnywhere, Category="Terreno|LOD")
	double LOD2Distance = 200000.0;

	UPROPERTY(EditAnywhere, Category="Terreno|HeightRange")
	double HeightMinCm = -1393.9;

	UPROPERTY(EditAnywhere, Category="Terreno|HeightRange")
	double HeightMaxCm = 64399.8;

	virtual void BeginPlay() override;
	virtual void Tick(float DeltaTime) override;

private:
	TArray<uint16> AlturasRAW;
	int32 NumChunksX = 0;
	int32 NumChunksY = 0;
	FVector OriginWorld = FVector::ZeroVector;

	struct FInfoChunk
	{
		int32 ChunkX, ChunkY;
		int32 OffsetPxX, OffsetPxY;
		int32 AnchoPx;
		int32 AltoPx;
		FVector CentroWorld;
		UPROPERTY() UProceduralMeshComponent* MeshLOD0 = nullptr;
		UPROPERTY() UProceduralMeshComponent* MeshLOD1 = nullptr;
		UPROPERTY() UProceduralMeshComponent* MeshLOD2 = nullptr;
	};
	TArray<FInfoChunk> ChunksInfo;

	void CargarRAW(const FString& Ruta);
	void GenerarChunk(FInfoChunk& Info, int32 LODLevel, int32 Step);
	void GenerarTodosChunks();

	UMaterialInterface* CrearMaterialTerreno();

	void BuildMeshData(const FInfoChunk& Info, int32 Step,
		TArray<FVector>& OutVerts, TArray<int32>& OutTris,
		TArray<FVector>& OutNormals, TArray<FVector4>& OutTangents,
		TArray<FVector2D>& OutUV0, TArray<FLinearColor>& OutColors) const;

	uint16 LeerAltura(int32 PxX, int32 PxY) const;
	float AlturaWorld(int32 PxX, int32 PxY) const;
	FVector CalcNormal(int32 PxX, int32 PxY) const;
	FVector4 CalcTangent(int32 PxX, int32 PxY) const;
	FLinearColor CalcVertexColor(int32 PxX, int32 PxY) const;

	void ActualizarLODs();

	FVector CentroMundo() const { return FVector(191800.0, 857000.0, 0.0); }
	double MitadMundo() const { return (ResolucionRAW - 1) * EscalaXY * 0.5; }
};
