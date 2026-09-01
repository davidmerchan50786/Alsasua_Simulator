#pragma once
#include "UObject/UObjectGlobals.h"
#include "Misc/PackageName.h"
#include "Materials/MaterialInterface.h"
#include "Materials/MaterialParameterCollection.h"
#include "Engine/StaticMesh.h"

/**
 * La ÚNICA colección de parámetros de material del proyecto.
 *
 * Había dos nombres circulando y sólo uno existe. /Game/Materiales/MPC_Clima lo
 * crean UCreadorMaterialEdificio y Tools/SetupMaterials.py, y es el que conduce
 * UClimaSubsystem en runtime. /Game/Materials/MPC_AlsasuaGlobal —otra carpeta,
 * en inglés— no lo crea nadie, y sin embargo lo cargaban cuatro sistemas de C++
 * y tres scripts de editor: el pintado de humedad del RVT, el director de
 * cámara, el gestor de efectos visuales y los materiales de charcos, ventanas
 * de noche y auto-textura de terreno. LoadObject devolvía null y toda esa capa
 * no hacía nada, sin más aviso que un warning por parámetro.
 *
 * Está aquí y no copiada en cada fichero porque cuatro copias de una ruta son
 * cuatro oportunidades de que una se quede atrás — que es exactamente lo que
 * pasó.
 */
inline const TCHAR* RutaMPCClima() { return TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"); }

/** Carga el MPC del clima. null si el proyecto no ha generado materiales aún. */
inline UMaterialParameterCollection* CargarMPCClima()
{
	return LoadObject<UMaterialParameterCollection>(nullptr, RutaMPCClima());
}

// Los materiales AAA de la libreria Road compilan solo si TODAS sus dependencias
// (atlases/surfaces de Megascans, MaterialFunctions, texturas SurfaceFeature y la
// VT mask) estan presentes en el proyecto. Si falta alguna, el MI se carga pero
// compila a Default Material (gris) sin que LoadObject falle. Estos gates detectan
// la ausencia para caer a los materiales propios en vez de renderizar gris.
//
// OJO CON LAS RUTAS: las cinco apuntaban a una disposicion de carpetas que ya
// no existe —/Game/Megascans/..., /Game/Material/MaterialFunction/...,
// /Game/Textures/...—. Megascans se importa bajo Content/External/Megascans/
// Megascans/ (el nombre se repite: es la carpeta del Bridge dentro de
// External) y las tres piezas de CitySample viven en UnrealDrive_CitySample.
// Con las rutas viejas DoesPackageExist fallaba siempre, asi que el gate daba
// falso aunque estuviera todo, y la libreria Road entera —asfaltos, aceras,
// bordillos, marcas— caia a los materiales planos de respaldo. Verificado
// contra el disco: son las rutas reales de los cinco assets.
inline bool MaterialesAAADisponibles()
{
	static const TCHAR* const Gates[] = {
		TEXT("/Game/External/Megascans/Megascans/Atlases/Debris_Manmade_00/shmpulh_8K_Albedo"),
		TEXT("/Game/External/Megascans/Megascans/Surfaces/Dirty_Sidewalk_2x2_M_00/ugxjcdpn_4K_Albedo"),
		TEXT("/Game/UnrealDrive_CitySample/MasterMaterials/MaterialFunction/MF_WorldNoise"),
		TEXT("/Game/UnrealDrive_CitySample/Textures/SurfaceFeature/T_AsphaltNoise_packedA"),
		TEXT("/Game/UnrealDrive_CitySample/Textures/Base/T_Black_Mask_VT"),
	};
	for (const TCHAR* G : Gates)
		if (!FPackageName::DoesPackageExist(G)) return false;
	return true;
}

inline UMaterialInterface* CargarMaterialConFallback(const TCHAR* RutaAAA, const TCHAR* RutaPropia, const TCHAR* RutaFinal)
{
	if (MaterialesAAADisponibles())
	{
		if (UMaterialInterface* AAA = LoadObject<UMaterialInterface>(nullptr, RutaAAA))
			return AAA;
	}
	UMaterialInterface* Propio = LoadObject<UMaterialInterface>(nullptr, RutaPropia);
	return Propio ? Propio : LoadObject<UMaterialInterface>(nullptr, RutaFinal);
}

inline UMaterialInterface* CargarMaterialConFallbackSeguro(const TCHAR* RutaAAA, const TCHAR* RutaPropia, const TCHAR* RutaFinal)
{
	if (UMaterialInterface* Mat = CargarMaterialConFallback(RutaAAA, RutaPropia, RutaFinal))
		return Mat;
	return LoadObject<UMaterialInterface>(nullptr, TEXT("/Engine/EngineMaterials/DefaultMaterial.DefaultMaterial"));
}

inline UStaticMesh* CargarMeshRapido(const TCHAR* MeshPath)
{
	return MeshPath ? LoadObject<UStaticMesh>(nullptr, MeshPath) : nullptr;
}

inline UMaterialInterface* CargarMaterialRapido(const TCHAR* MaterialPath)
{
	return MaterialPath ? LoadObject<UMaterialInterface>(nullptr, MaterialPath) : nullptr;
}
