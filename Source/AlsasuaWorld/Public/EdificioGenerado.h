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

USTRUCT(BlueprintType)
struct FFachadaConfig
{
	GENERATED_BODY()

	// Ventanas: separación horizontal/vertical entre centros de ventana (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float EspaciadoX = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float EspaciadoY = 320.f;

	// Tamaño de cada ventana (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float AnchoVentana = 120.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float AltoVentana = 160.f;

	// Altura del primer piso sobre el suelo (cm) — la primera fila de ventanas empieza aquí.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float OffsetPrimerPiso = 180.f;

	// Cornisa: altura de la franja decorativa bajo el alero (cm).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float AltoCornisa = 30.f;

	// Puerta en la fachada más larga.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	bool bPonerPuerta = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float AnchoPuerta = 140.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float AltoPuerta = 260.f;

	// Profundidad del rebaje de ventana/puerta (cm) — da sombra y relieve.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	float ProfundidadRebaje = 15.f;

	// Colores vertex (requiere material con vertex color).
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	FColor ColorMuro = FColor(196, 142, 112);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	FColor ColorVentana = FColor(40, 55, 75);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	FColor ColorPuerta = FColor(80, 50, 30);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	FColor ColorCornisa = FColor(210, 190, 170);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Fachada")
	FColor ColorZocalo = FColor(120, 100, 85);
};

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

	// Versión con detalle de fachada: ventanas, puerta, cornisa, zócalo.
	void ConstruirConDetalle(const TArray<FVector2D>& FootprintLocal, float AlturaCm,
	                         const FFachadaConfig& Config,
	                         FColor ColorTejado = FColor(150, 70, 48),
	                         EFormaTejado Forma = EFormaTejado::Cuatro_Aguas,
	                         FVector2D EjeCaballete = FVector2D(1, 0), float EscalaTejado = 1.f);

private:
	void TejadoCuatroAguas(const TArray<FVector2D>& P, float AlturaCm, float RoofH, FColor Color,
	                       TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, TArray<FVector2D>& UV, TArray<FColor>& C);
	void TejadoDosAguas(const TArray<FVector2D>& P, float AlturaCm, float RoofH, FVector2D Eje, FColor Color,
	                    TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N, TArray<FVector2D>& UV, TArray<FColor>& C);

	// Fachada helper: genera muro con ventanas/puerta/cornisa/zócalo.
	void GenerarFachadaConDetalle(const TArray<FVector2D>& P, float Alero, const FFachadaConfig& Config,
	                              TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N,
	                              TArray<FVector2D>& UV, TArray<FColor>& C);

public:
	UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* Malla;

	UPROPERTY(EditAnywhere, Category="Edificio") int32 Id = 0;
	UPROPERTY(EditAnywhere, Category="Edificio") FString NombreEdificio;
	UPROPERTY(EditAnywhere, Category="Edificio") int32 Plantas = 1;
	UPROPERTY(EditAnywhere, Category="Edificio") bool bDetalleActivo = false;
	UPROPERTY(EditAnywhere, Category="Edificio") FFachadaConfig ConfigFachada;
};
