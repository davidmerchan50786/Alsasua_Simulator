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
	void GenerarTodoAhora();

	float AlturaEnMundo(float X, float Y) const;
	FVector NormalEnMundo(float X, float Y) const;

	/** Rectángulo XY (cm) que cubre el terreno; fuera de él no hay malla. */
	FBox2D BoundsXY() const;

	UPROPERTY(EditAnywhere, Category="Terreno") int32 SizeChunkPx = 128;
	UPROPERTY(EditAnywhere, Category="Terreno") int32 ResolucionRAW = 4033;
	UPROPERTY(EditAnywhere, Category="Terreno") double EscalaXY = 178.5714;
	UPROPERTY(EditAnywhere, Category="Terreno") double EscalaZ = 200.0;
	UPROPERTY(EditAnywhere, Category="Terreno") double LocZ = 49567.0;
	UPROPERTY(EditAnywhere, Category="Terreno") FString RutaRAW = TEXT("Terreno/alsasua_landscape_4033.r16");

	/** LIDAR 0.5m real (Alsasua centro urbano) — mayor precisión, se funde con el DTM ancho en los bordes. */
	UPROPERTY(EditAnywhere, Category="Terreno|LIDAR") bool bUsarLidar = true;
	UPROPERTY(EditAnywhere, Category="Terreno|LIDAR") FString RutaLidarRAW = TEXT("Terreno/lidar_dtm_05m.raw");
	UPROPERTY(EditAnywhere, Category="Terreno|LIDAR") FString RutaLidarMeta = TEXT("Terreno/lidar_dtm_meta.json");
	UPROPERTY(EditAnywhere, Category="Terreno|LIDAR") FString RutaLidarGround = TEXT("Terreno/lidar_ground.xyz");
	/** Ancho del blend gaussiano (metros) entre LIDAR y DTM ancho en los bordes del área LIDAR. */
	UPROPERTY(EditAnywhere, Category="Terreno|LIDAR") double LidarBlendMetros = 200.0;

	/** Valida el heightmap final contra lidar_ground.xyz (587k puntos reales) y loguea RMSE. */
	UFUNCTION(CallInEditor, Category="Terreno|LIDAR")
	void ValidarTerrenoRMSE();

	UPROPERTY(EditAnywhere, Category="Terreno|Material")
	TSoftObjectPtr<UMaterialInterface> TerrainMaterial;

	/** Ortofoto satelital real de Alsasua, draped sobre el terreno completo. */
	UPROPERTY(EditAnywhere, Category="Terreno|Material")
	TSoftObjectPtr<UTexture> SatelliteImage;

	UPROPERTY(EditAnywhere, Category="Terreno|Material|Textures")
	TSoftObjectPtr<UTexture> GrassDiffuse;

	UPROPERTY(EditAnywhere, Category="Terreno|Material|Textures")
	TSoftObjectPtr<UTexture> GrassNormal;

	UPROPERTY(EditAnywhere, Category="Terreno|Material|Textures")
	TSoftObjectPtr<UTexture> RockDiffuse;

	UPROPERTY(EditAnywhere, Category="Terreno|Material|Textures")
	TSoftObjectPtr<UTexture> RockNormal;

	UPROPERTY(EditAnywhere, Category="Terreno|Material|Textures")
	TSoftObjectPtr<UTexture> GroundDiffuse;

	UPROPERTY(EditAnywhere, Category="Terreno|Material|Textures")
	TSoftObjectPtr<UTexture> GroundNormal;

	UPROPERTY(EditAnywhere, Category="Terreno|Material|Textures")
	float TextureTilingCm = 500.0f;

	UPROPERTY(EditAnywhere, Category="Terreno|LOD")
	int32 LOD0Step = 1;

	UPROPERTY(EditAnywhere, Category="Terreno|LOD")
	int32 LOD1Step = 4;

	UPROPERTY(EditAnywhere, Category="Terreno|LOD")
	int32 LOD2Step = 16;

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
	void GenerarSiguienteChunk();

	TArray<uint16> AlturasRAW;
	int32 NumChunksX = 0;
	int32 NumChunksY = 0;
	FVector OriginWorld = FVector::ZeroVector;
	int32 ChunkIndexProgreso = 0;

	// ── LIDAR 0.5m ──────────────────────────────────────────────────────
	TArray<uint16> LidarRaw;
	int32 LidarRes = 0;
	double LidarWidthM = 0.0;
	double LidarLengthM = 0.0;
	double LidarZMinM = 0.0;
	double LidarZMaxM = 0.0;
	bool bLidarCargado = false;
	bool bLidarFusionado = false;

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
	void CargarLidar();
	float SampleLidarBicubic(double WorldXcm, double WorldYcm) const;
	bool DentroLidar(double WorldXcm, double WorldYcm, double& OutDistBordeCm) const;
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

public:
	// Públicos porque ATerrenoLejano necesita saber dónde acaba este terreno para
	// dejar ahí el agujero de su anillo. Son getters puros; que la fórmula viva en
	// un solo sitio evita que el anillo y el terreno se desalineen si cambia la
	// resolución o la escala.
	FVector CentroMundo() const { return FVector(191800.0, 857000.0, 0.0); }
	double MitadMundo() const { return (ResolucionRAW - 1) * EscalaXY * 0.5; }
};
