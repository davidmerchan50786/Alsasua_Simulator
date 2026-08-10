// CreadorPBRComun.h (sólo editor)
// Cableado PBR compartido por los materiales del pueblo.
//
// Content/Textures trae sets completos de ambientCG (Color / Normal / Roughness
// y, en la mayoría, AO). Cada CreadorMaterial* repetía el mismo cableado de UV y
// muestreo, y entre todos dejaban sin usar los once mapas de roughness y los
// sets de Brick, Concrete, Wood, MetalPlate, Grass y Ground.
#pragma once

#include "CoreMinimal.h"

class UMaterial;
class UMaterialExpression;

namespace AlsasuaPBR
{
	/** Sets disponibles en Content/Textures (sin el prefijo T_ ni el sufijo). */
	struct FOpciones
	{
		/** "Asphalt", "Brick", "Cobblestone", "Concrete", "Grass", "Ground",
		 *  "MetalPlate", "RoofTiles", "StoneWall", "Wood". */
		FString Set;

		/** Centímetros que ocupa una repetición de la textura. */
		float TileCm = 200.f;

		/** Escala sobre el mapa de roughness (1 = tal cual viene). */
		float EscalaRoughness = 1.f;

		/** Roughness con Wetness = 1: mojado, la superficie refleja. */
		float RoughnessMojado = 0.12f;

		/** Multiplica el color base. Para fachadas se pasa el VertexColor. */
		UMaterialExpression* Tinte = nullptr;

		/** Cablear el AO si el set lo trae. */
		bool bUsarAO = true;
	};

	/**
	 * Cablea BaseColor, Normal, Roughness (modulada por Wetness del MPC_Clima) y
	 * AO desde el set indicado, con UV en espacio de mundo.
	 * Devuelve false si no encuentra el mapa de color.
	 */
	bool Cablear(UMaterial* Mat, const FOpciones& Op);

	/** UV en espacio de mundo XY con teselado en cm. */
	UMaterialExpression* UVMundo(UMaterial* Mat, float TileCm, int32 X, int32 Y);

	/**
	 * Escalar del MPC_Clima con su ParameterId resuelto. Sin resolverlo el nodo
	 * se compila pero no lee el valor de la colección.
	 */
	UMaterialExpression* Clima(UMaterial* Mat, const TCHAR* Parametro, int32 X, int32 Y);
}
