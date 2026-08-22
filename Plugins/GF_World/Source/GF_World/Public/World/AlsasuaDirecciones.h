// AlsasuaDirecciones.h
// Calle y portal de cada edificio, y el punto de su calle.
//
// Datos/direcciones_osm.json lo genera Tools/ExtraerDireccionesOSM.py con los
// addr:street/addr:housenumber de OSM: 463 edificios con calle, 454 con portal,
// todos con id de way que casa con buildings_final.json.
//
// El punto de la calle viene ya resuelto en el fichero, buscado entre todos los
// tramos de roads_unity.json con ese nombre (374 edificios, a 14,9 m de mediana
// del eje). Aquí no se emparejan nombres ni se vuelven a parsear las 489 vías:
// normalizar nombres con acentos en tiempo de ejecución es la clase de cosa que
// falla en silencio y deja el dato sin usar.
#pragma once

#include "CoreMinimal.h"
#include "Math/RandomStream.h"

namespace AlsasuaDirecciones
{
	struct FDireccion
	{
		FString Calle;
		FString Portal;    // vacío si OSM no lo tiene

		/**
		 * Punto del eje de la calle más cercano al edificio, en las coordenadas
		 * de los JSON: local relativo en metros, con X = x e Y = z de Unity.
		 * Convertir con UAlsasuaGeoData::RelLocalToUE5.
		 */
		FVector2D PuntoCalle = FVector2D::ZeroVector;

		/** Falso si no se sabe dónde está su calle (o queda a más de 60 m). */
		bool bTienePuntoCalle = false;
	};

	/** Dirección del edificio, o null si no tiene ninguna en OSM. */
	GF_WORLD_API const FDireccion* De(int32 IdEdificio);

	/** Fachada de un edificio: centro de un lado de su caja y hacia dónde mira. */
	struct FFachada
	{
		/** Centro del lado, en local relativo (X = x, Y = z de los JSON). */
		FVector2D Punto = FVector2D::ZeroVector;

		/** Yaw de UE5 que mira hacia afuera del edificio (+X este, +Y norte). */
		float Yaw = 0.0f;

		/** Vector unitario 2D hacia afuera, en local relativo. */
		FVector2D Fuera = FVector2D(1.0f, 0.0f);

		/** true si el lado se eligió por su addr:street; false si por el lado largo. */
		bool bHaciaCalle = false;
	};

	/**
	 * Lado por el que da a la calle un edificio, dada la caja de su footprint.
	 *
	 * Antes esto era una moneda al aire dentro del sistema de puertas: dos FRand
	 * por edificio, así que la entrada cambiaba de fachada en cada arranque. Con
	 * el punto de calle de OSM se elige el lado más cercano al eje (374
	 * edificios); el resto cae al lado largo, con el sentido sorteado.
	 *
	 * Vive aquí y no en el sistema de puertas porque la fachada que da a la
	 * calle es la misma para todo lo que se cuelga de ella —puerta, portal,
	 * puerta de garaje, escaparate— y tenerla en dos sitios es garantizar que se
	 * separen.
	 *
	 * @param Sorteo  Determinista, sembrado por id: sin él el pueblo cambia en
	 *                cada arranque y no se puede razonar sobre lo que se ve.
	 */
	GF_WORLD_API FFachada LadoDeEntrada(int32 IdEdificio, const FVector2D& Min2,
		const FVector2D& Max2, FRandomStream& Sorteo);
}
