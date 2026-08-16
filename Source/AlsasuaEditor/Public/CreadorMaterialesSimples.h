// CreadorMaterialesSimples.h (capa EDITOR)
// Crea de una tacada los materiales sueltos que el runtime carga por ruta y que
// no generaba nadie.
//
// Veintinueve rutas de /Game/Materiales aparecían en LoadObject de una docena de
// sistemas —M_Madera, M_Piedra, M_Metal_Guardia, M_Toldo, los M_Asphalt_* por
// barrio…— sin que ningún generador las creara. No rompían nada: todos esos
// cargadores comprueban el null y siguen, así que la pieza salía con el material
// que trajera su malla. Pero la función visual no se veía nunca, y la auditoría
// las daba por buenas porque la carpeta sí la genera el proyecto.
//
// No son colores planos: cada uno se cablea con AlsasuaPBR::Cablear sobre uno de
// los sets de Content/Textures (Concrete, Wood, MetalPlate, Cobblestone, Asphalt,
// Ground, StoneWall, Grass) con su tinte, su teselado y su respuesta a la lluvia
// del MPC_Clima, igual que el resto de materiales del pueblo. Si el set no está
// descargado, Cablear devuelve false y se cae a color liso con su rugosidad, que
// sigue siendo mejor que el material por defecto.
//
// ORDEN: como todos los demás, necesita que exista MPC_Clima. Lo crea
// UCreadorMaterialEdificio::CrearMaterialEdificio(); si no está, estos salen
// grises.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CreadorMaterialesSimples.generated.h"

UCLASS()
class UCreadorMaterialesSimples : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Crea los 29. Devuelve cuántos han salido bien. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|Materiales")
	static int32 CrearMaterialesSueltos();

	/** Lo mismo, con la firma bool que espera la tabla de pasos de
	 *  UAlsasuaAssetGenerator. */
	static bool CrearSueltos();
};
