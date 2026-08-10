// AlsasuaTejadoModular.h (capa WORLD)
// Remate de tejado con el kit modular de Village (familia Roof_, 29 piezas).
//
// AEdificioGenerado ya construye el faldón: extruye el footprint real de
// buildings_final.json y lo remata a dos aguas, cuatro aguas o plano según el
// LIDAR de cada edificio. Lo que le falta es el borde: un faldón procedural
// termina en una arista limpia, y un tejado de verdad tiene vuelo de alero,
// teja de cumbrera, piezas de esquina y limatesas.
//
// Este sistema no rehace la geometría: recorre los AEdificioGenerado ya
// construidos y coloca las piezas del kit sobre su propio perímetro, su
// cumbrera y sus limas, leyendo la ficha (FTejadoConstruido) que el edificio
// anota al construirse. Así no repite la conversión de coordenadas ni el
// muestreo de terreno, que es donde se descuadran los dos extremos.
//
// El largo de cada pieza se mide del bounding box de la malla importada, no se
// asume: el teselado cuadra con cualquier kit que haya, y cada pieza se estira
// lo justo para que la hilada no deje juntas abiertas.
//
// Si el kit no está importado, Resolver devuelve null y el sistema no coloca
// nada. Es deliberado: un cubo del motor repetido 40.000 veces a lo largo de
// los aleros es peor que no tener remate.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaTejadoModular.generated.h"

class AEdificioGenerado;
class UHierarchicalInstancedStaticMeshComponent;

UCLASS()
class ALSASUAWORLD_API UAlsasuaTejadoModular : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Vuelo del alero sobre la fachada (cm). */
	UPROPERTY(EditAnywhere, Category = "Tejado")
	float VueloAlero = 35.f;

	/**
	 * No se remata por debajo de este perímetro. Es una válvula: el edificio
	 * más pequeño de buildings_final.json tiene 12,1 m de perímetro, así que
	 * con los datos reales no se salta ninguno.
	 */
	UPROPERTY(EditAnywhere, Category = "Tejado")
	float PerimetroMinimo = 1200.f;

	/** Fracción de edificios con chimenea, sorteada por id (determinista). */
	UPROPERTY(EditAnywhere, Category = "Tejado", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FraccionChimenea = 0.45f;

	/**
	 * Distancia a la que se dejan de dibujar las piezas (cm). Son entre 40.000
	 * y 150.000 instancias según lo que mida la pieza del kit (comprobado sobre
	 * los 1030 footprints), y a 300 m un remate de tejado ya no se distingue.
	 */
	UPROPERTY(EditAnywhere, Category = "Tejado")
	float DistanciaCulling = 30000.f;

	UFUNCTION(BlueprintCallable, Category = "Tejado")
	int32 Cargar();

	/** Cuántos edificios se remataron y cuántas piezas se colocaron. */
	UPROPERTY(BlueprintReadOnly, Category = "Tejado")
	int32 EdificiosRematados = 0;

	UPROPERTY(BlueprintReadOnly, Category = "Tejado")
	int32 PiezasColocadas = 0;

private:
	/**
	 * Una pieza del kit, normalizada por su bounding box.
	 *
	 * Ancla es el punto de la malla que se hace coincidir con el punto de
	 * colocación: centrado en horizontal y por la base en vertical. Las piezas
	 * de un kit modular vienen pivotadas en la esquina de su celda de rejilla,
	 * y usar el pivote crudo desplaza la hilada media pieza.
	 */
	struct FPieza
	{
		UHierarchicalInstancedStaticMeshComponent* ISM = nullptr;
		float Largo = 100.f;      // cm que cubre a lo largo de la hilada
		bool bRunEnY = false;     // el eje que corre es Y en vez de X
		FVector Ancla = FVector::ZeroVector;
	};

	/** Crea (una vez) el ISM del tipo, o null si el kit no tiene esa pieza. */
	FPieza* Obtener(const FString& Tipo);

	void RematarEdificio(AEdificioGenerado* Edificio);

	/** Tiende una hilada de A a B, estirando las piezas para llenar exacto. */
	void TenderPiezas(FPieza& P, const FVector& A, const FVector& B);

	/** Una sola pieza en Punto, con su eje de recorrido mirando a Direccion. */
	void ColocarUna(FPieza& P, const FVector& Punto, const FVector& Direccion, float EscalaLargo);

	UPROPERTY() AActor* Host = nullptr;
	TMap<FString, FPieza> Piezas;
	bool bHecho = false;
};
