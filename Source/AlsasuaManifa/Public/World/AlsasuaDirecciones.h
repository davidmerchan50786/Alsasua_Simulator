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
	ALSASUAMANIFA_API const FDireccion* De(int32 IdEdificio);
}
