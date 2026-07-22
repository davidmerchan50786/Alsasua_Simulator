// ImportadorLandscape.h (capa EDITOR, sólo editor)
// Importa el heightmap reencodeado (alsasua_landscape_4033.r16) como un
// ALandscape con la transformada exacta calculada por Tools/PrepararLandscape.py,
// de modo que la Z del mundo = (altitudReal - Z_MIN) * 100 cm.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "ImportadorLandscape.generated.h"

UCLASS()
class UImportadorLandscape : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/**
	 * Crea un ALandscape en el mundo del editor desde un heightmap RAW uint16.
	 * Valores por defecto = los de Tools/PrepararLandscape.py (4033, escala UTM real).
	 * @param RutaR16      ruta absoluta al .r16 (4033^2, uint16 LE).
	 * @param Resolucion   verts por lado (4033).
	 * @param EscalaXY_cm  cm por quad (178.5714).
	 * @param EscalaZ      escala Z del actor (200).
	 * @param LocZ_cm      Z del actor en cm (49567).
	 * @param CentroXY_cm  centro del Landscape en XY (Herriko Plaza). Si ZeroVector, deja (0,0).
	 */
	// En un nivel World Partition, el import de un único ALandscape no genera los
	// streaming proxies: por defecto avisa y aborta. Pon bPermitirWorldPartition=true
	// solo si sabes lo que haces (o usa Landscape Mode -> Import from File, que sí
	// crea los proxies).
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Terreno")
	static bool ImportarLandscape(
		const FString& RutaR16,
		int32 Resolucion   = 4033,
		double EscalaXY_cm = 178.5714,
		double EscalaZ     = 200.0,
		double LocZ_cm     = 49567.0,
		FVector CentroXY_cm = FVector::ZeroVector,
		bool bPermitirWorldPartition = false);
};
