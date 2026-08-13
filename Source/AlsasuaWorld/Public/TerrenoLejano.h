// TerrenoLejano.h (capa WORLD)
// Anillo de relieve real alrededor del terreno jugable, para que el mundo no se
// corte en seco a 3,6 km del centro: al oeste Aizkorri, al este San Donato/Andía,
// al norte Aralar, al sur Urbasa/Lokiz.
//
// Es un cuadrado de 60x60 km con un agujero central del tamaño exacto del terreno
// de ATerrenoGenerado, así que no se solapan ni pelean en Z. Los datos salen de
// Content/Terreno/alsasua_relieve_lejano_2048.r16 (MDT25 del IGN, ver
// Tools/DescargarRelieveLejano.py), con su caja y su codificación en el _meta.json
// de al lado — no están hardcodeadas aquí.
//
// Es decorado, no suelo: sin colisión (si la tuviera, los muestreos de altura por
// LineTrace de árboles y suelos poligonales podrían engancharse a él) y sin sombras
// dinámicas, que a esta distancia no aportan y salen caras.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TerrenoLejano.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ALSASUAWORLD_API ATerrenoLejano : public AActor
{
	GENERATED_BODY()

public:
	ATerrenoLejano();

	virtual void BeginPlay() override;

	/** Lee el .r16 + meta y construye el anillo. Devuelve los triángulos emitidos. */
	UFUNCTION(BlueprintCallable, Category="Alsasua|Terreno")
	int32 Construir();

	UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* Malla;

	/** Lado de celda en metros. Sube para gastar menos, baja para menos facetado.
	 *  Tiene que dividir exacto al semilado del agujero (360000 cm) o el borde no
	 *  cae en línea de rejilla y aparece un diente. */
	UPROPERTY(EditAnywhere, Category="Terreno|Lejano") float CeldaM = 150.f;

	/** Ancho de la banda donde la altura se funde del borde del terreno jugable a
	 *  la del MDT. Sin banda, los pocos metros de diferencia entre ambas fuentes se
	 *  ven como un escalón alrededor de todo el pueblo. */
	UPROPERTY(EditAnywhere, Category="Terreno|Lejano") float BandaFusionM = 1200.f;

	UPROPERTY(EditAnywhere, Category="Terreno|Lejano") FString RutaRAW =
		TEXT("Terreno/alsasua_relieve_lejano_2048.r16");
	UPROPERTY(EditAnywhere, Category="Terreno|Lejano") FString RutaMeta =
		TEXT("Terreno/alsasua_relieve_lejano_meta.json");

private:
	// Leídos del meta, en cm de mundo salvo donde se diga.
	int32 Resolucion = 0;
	double CentroX = 0.0, CentroY = 0.0, SemiladoCm = 0.0;
	double DatumM = 0.0, PasoM = 1.0 / 32.0;
	TArray<uint16> Alturas;

	bool CargarDatos();
	/** Altura del MDT en un punto de mundo (cm). Bilineal; fuera de caja, borde. */
	float AlturaMDT(double MundoX, double MundoY) const;
};
