#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrenoGenerado.generated.h"

class UProceduralMeshComponent;

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

	virtual void BeginPlay() override;

private:
	TArray<uint16> AlturasRAW;
	int32 NumChunksX = 0;
	int32 NumChunksY = 0;

	struct FInfoChunk
	{
		int32 ChunkX, ChunkY;
		int32 OffsetPxX, OffsetPxY;
		int32 AnchoPx;
		FVector CentroLocal;
	};
	TArray<FInfoChunk> ChunksInfo;

	void CargarRAW(const FString& Ruta);
	void GenerarChunk(const FInfoChunk& Info);
	void GenerarColisionChunk(const FInfoChunk& Info);
	uint16 LeerAltura(int32 PxX, int32 PxY) const;
	float AlturaWorld(int32 PxX, int32 PxY) const;
	FVector CalcNormal(int32 PxX, int32 PxY) const;
};
