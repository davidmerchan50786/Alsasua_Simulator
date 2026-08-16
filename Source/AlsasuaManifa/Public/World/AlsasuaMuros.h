// AlsasuaMuros.h (capa MANIFA)
// Los muros del pueblo: cada tramo del perímetro de los 1030 footprints de
// buildings_final.json, con su largo y su normal saliente.
//
// Existe porque tres sistemas cuelgan cosas de una pared y ninguno tenía la
// pared: AlsasuaStreetArtSystem pintaba los 23 murales y grafitis en el
// centroide del barrio, AlsasuaPaintedStreetSignSystem clavaba las placas de
// calle en el eje de la calzada a 2,5 m de altura, y AlsasuaShopFrontSystem
// ponía el escaparate con un giro sorteado. Los tres flotaban, y los tres
// miraban a donde caía.
//
// Se calcula una vez y se guarda: son ~9000 tramos y recorrer los footprints
// tres veces no tiene sentido. La normal se decide contra el centroide del
// footprint, que es lo único que distingue afuera de adentro sin conocer el
// sentido de giro del polígono — y buildings_final.json no lo garantiza.
#pragma once

#include "CoreMinimal.h"

namespace AlsasuaMuros
{
	struct FMuro
	{
		int32 EdificioId = -1;
		FString Barrio;

		/** Extremos del tramo, en local relativo (X = x, Y = z de los JSON). */
		FVector2D A = FVector2D::ZeroVector;
		FVector2D B = FVector2D::ZeroVector;

		float LargoM = 0.0f;

		/** Normal saliente, unitaria, en local relativo. */
		FVector2D Fuera = FVector2D(1.0f, 0.0f);

		/** Yaw de UE5 que mira hacia afuera del muro. */
		float Yaw = 0.0f;

		/** Punto medio del tramo, en local relativo. */
		FVector2D Medio() const { return (A + B) * 0.5f; }
	};

	/** Todos los tramos, leídos de buildings_final.json la primera vez. */
	ALSASUAMANIFA_API const TArray<FMuro>& Todos();

	/** Índices de los tramos de al menos ese largo. Un mural de 5 m no cabe en
	 *  una medianera de 3, y pedirlo aparte evita comprobarlo en cada sorteo. */
	ALSASUAMANIFA_API void DeAlMenos(float LargoMinimoM, TArray<int32>& OutIndices);

	/**
	 * Tramo más cercano a un punto, o null si no hay ninguno dentro del radio.
	 *
	 * Para lo que se cuelga cerca de algo que ya está situado: la placa de una
	 * calle va en la esquina que da a esa calle, no en un muro cualquiera.
	 *
	 * @param PuntoRel    Punto en local relativo.
	 * @param RadioMaxM   Más allá de esto se devuelve null, para no acabar
	 *                    pintando la placa de una calle en la casa de otra.
	 * @param LargoMinimoM Descarta tramos demasiado cortos para lo que va encima.
	 */
	ALSASUAMANIFA_API const FMuro* MasCercano(const FVector2D& PuntoRel, float RadioMaxM,
		float LargoMinimoM = 0.0f);

	/** Olvida la caché. Para reimportar el dataset sin reiniciar el editor. */
	ALSASUAMANIFA_API void LimpiarCache();
}
