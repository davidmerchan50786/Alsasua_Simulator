// AlsasuaRedViaria.h (capa MANIFA)
// Grafo de calzada del pueblo: nodos en los cruces y tramos dirigidos entre
// ellos, con ancho, tipo y sentido. Puerto de nada — esto no existía.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaRedViaria.generated.h"

/** Un tramo dirigido entre dos nodos. */
USTRUCT(BlueprintType)
struct FTramoViario
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) int32 NodoA = 0;
	UPROPERTY(BlueprintReadOnly) int32 NodoB = 0;
	/** Ancho de calzada en cm, de "width" del dato (metros). */
	UPROPERTY(BlueprintReadOnly) float AnchoCm = 500.f;
	UPROPERTY(BlueprintReadOnly) FString Tipo;
	UPROPERTY(BlueprintReadOnly) FString Nombre;
	UPROPERTY(BlueprintReadOnly) int32 IdVia = 0;
	UPROPERTY(BlueprintReadOnly) float LargoCm = 0.f;
};

/**
 * Red viaria: el grafo por el que se circula.
 *
 * Antes no había grafo. UAlsasuaDynamicTrafficSystem y
 * UAlsasuaNPCPedestrianSystem leían roads_unity.json cada uno por su cuenta y se
 * quedaban una lista de polilíneas sueltas: un coche recorría UNA calle y al
 * llegar al final volvía de un salto al punto 0, porque no había por dónde
 * girar. Tampoco filtraban por tipo —el 'type' se leía a una variable y no se
 * usaba—, así que circulaban por las 87 vías peatonales; y descartaban toda vía
 * con menos de cuatro puntos, que son 192 de las 489, el 39%.
 *
 * Los cruces salen por coincidencia EXACTA de coordenada, sin encaje por
 * proximidad: los datos vienen de OSM y sólo 3 de las 402 vías conducibles
 * quedan sin conectar así. Tools/VerificarRedViaria.py replica este cálculo y
 * saca los números que tiene que dar el log de abajo; si no coinciden, aquí se
 * está filtrando distinto.
 */
UCLASS()
class GF_CARRETERAS_API UAlsasuaRedViaria : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	/** Construye el grafo. Idempotente: la segunda llamada no hace nada. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Red")
	int32 Construir();

	UFUNCTION(BlueprintPure, Category = "Alsasua|Red") bool EstaLista() const { return Tramos.Num() > 0; }
	UFUNCTION(BlueprintPure, Category = "Alsasua|Red") int32 NumNodos() const { return Nodos.Num(); }
	UFUNCTION(BlueprintPure, Category = "Alsasua|Red") int32 NumTramos() const { return Tramos.Num(); }

	/** Posición de mundo de un nodo (cm), ya apoyada en el terreno. */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Red")
	FVector PosicionNodo(int32 Nodo) const;

	const FTramoViario& Tramo(int32 Indice) const { return Tramos[Indice]; }

	/**
	 * Los dos criterios de tipo de vía, en un sitio y no en cada sistema.
	 *
	 * Cada uno se lo montaba por su cuenta y los dos se equivocaban al revés: el
	 * tráfico leía el 'type' y no lo usaba, así que los coches circulaban por
	 * las 87 vías peatonales; y el de peatones no filtraba nada, así que había
	 * gente andando por las 39 vías de autovía y sus 50 enlaces.
	 */
	UFUNCTION(BlueprintPure, Category = "Alsasua|Red")
	static bool EsConducible(const FString& Tipo);

	UFUNCTION(BlueprintPure, Category = "Alsasua|Red")
	static bool EsTransitableAPie(const FString& Tipo);

	/** Un tramo cualquiera, para colocar algo que circula. -1 si no hay red. */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Red")
	int32 TramoAleatorio(int32 Semilla) const;

	/**
	 * Continuación al llegar al final de un tramo.
	 *
	 * Evita el giro de 180º salvo que el nodo sea un fondo de saco, que es lo
	 * que hace que un coche parezca que circula en vez de rebotar. Devuelve -1
	 * si el nodo no tiene salida.
	 */
	UFUNCTION(BlueprintCallable, Category = "Alsasua|Red")
	int32 SiguienteTramo(int32 TramoActual, int32 Semilla) const;

private:
	/** Posiciones de nodo en coordenadas de mundo (cm). */
	TArray<FVector> Nodos;
	TArray<FTramoViario> Tramos;
	/** Por nodo, los índices de tramo que SALEN de él. */
	TArray<TArray<int32>> Salidas;

};
