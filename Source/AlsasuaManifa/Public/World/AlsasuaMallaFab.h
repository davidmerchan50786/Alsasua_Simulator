// AlsasuaMallaFab.h
// Resuelve qué malla usar para cada pieza del pueblo, por orden de calidad:
//
//   1. Lo descargado de Fab / biblioteca de Epic, buscado por palabra clave.
//   2. Las mallas propias de /Game/Mobiliario (UCreadorMallaMobiliario).
//   3. La forma básica del motor, como último recurso para que algo se vea.
//
// Así el contenido de Fab entra en el mundo en cuanto existe, sin tocar los
// sistemas de colocación ni volver a generar nada.
#pragma once

#include "CoreMinimal.h"

class UStaticMesh;

namespace AlsasuaMallaFab
{
	/**
	 * Malla para un tipo de street_furniture.json ("banco", "papelera",
	 * "bollard"...). FormaBasica es la ruta del motor a la que caer.
	 * Devuelve null sólo si tampoco existe la forma básica.
	 */
	ALSASUAMANIFA_API UStaticMesh* Resolver(const FString& Tipo, const TCHAR* FormaBasica);

	/** ¿La malla devuelta para ese tipo viene de Fab? Para decidir si escalar. */
	ALSASUAMANIFA_API bool VieneDeFab(const FString& Tipo);

	/** Olvida la caché: tras importar de Fab sin reiniciar el editor. */
	ALSASUAMANIFA_API void LimpiarCache();
}
