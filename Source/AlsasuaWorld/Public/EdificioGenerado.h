// EdificioGenerado.h (capa WORLD)
// Edificio procedural: extruye un footprint (polígono) a una altura dada y
// genera muros + tapas (suelo/tejado) con un ProceduralMeshComponent.
// Puerto de GeneradorGeometriaPrecisa / SistemaEdificiosAAA (geometría base).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "EdificioGenerado.generated.h"

class UProceduralMeshComponent;

UENUM()
enum class EFormaTejado : uint8 { Plano, Cuatro_Aguas, Dos_Aguas };

UCLASS()
class ALSASUAWORLD_API AEdificioGenerado : public AActor
{
	GENERATED_BODY()

public:
	AEdificioGenerado();

	// Footprint en XY locales (cm, relativo al origen del actor). Altura en cm.
	// El polígono debe ser simple (sin auto-intersección); se cierra solo.
	// ColorMuro/ColorTejado tiñen por vértice (requiere material con vertex color).
	// Forma/EjeCaballete/EscalaTejado vienen de los datos LIDAR reales por edificio.
	void Construir(const TArray<FVector2D>& FootprintLocal, float AlturaCm,
	               FColor ColorMuro = FColor(196, 142, 112), FColor ColorTejado = FColor(150, 70, 48),
	               EFormaTejado Forma = EFormaTejado::Cuatro_Aguas,
	               FVector2D EjeCaballete = FVector2D(1, 0), float EscalaTejado = 1.f);

private:
	void TejadoCuatroAguas(const TArray<FVector2D>& P, float AlturaCm, float RoofH, FColor Color,
	                       TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, TArray<FVector2D>& UV, TArray<FColor>& C);
	void TejadoDosAguas(const TArray<FVector2D>& P, float AlturaCm, float RoofH, FVector2D Eje, FColor Color,
	                    TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, TArray<FVector2D>& UV, TArray<FColor>& C);

public:
	UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* Malla;

	UPROPERTY(EditAnywhere, Category="Edificio") int32 Id = 0;
	UPROPERTY(EditAnywhere, Category="Edificio") FString NombreEdificio;
	UPROPERTY(EditAnywhere, Category="Edificio") int32 Plantas = 1;
};
