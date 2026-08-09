#pragma once
#include "UObject/UObjectGlobals.h"
#include "Misc/PackageName.h"
#include "Materials/MaterialInterface.h"
#include "Engine/StaticMesh.h"

// Los materiales AAA de la libreria Road compilan solo si TODAS sus dependencias
// (atlases/surfaces de Megascans, MaterialFunctions, texturas SurfaceFeature y la
// VT mask) estan presentes en el proyecto. Si falta alguna, el MI se carga pero
// compila a Default Material (gris) sin que LoadObject falle. Estos gates detectan
// la ausencia para caer a los materiales propios en vez de renderizar gris.
inline bool MaterialesAAADisponibles()
{
	static const TCHAR* const Gates[] = {
		TEXT("/Game/Megascans/Atlases/Debris_Manmade_00/shmpulh_8K_Albedo"),
		TEXT("/Game/Megascans/Surfaces/Dirty_Sidewalk_2x2_M_00/ugxjcdpn_4K_Albedo"),
		TEXT("/Game/Material/MaterialFunction/MF_WorldNoise"),
		TEXT("/Game/Textures/SurfaceFeature/T_AsphaltNoise_packedA"),
		TEXT("/Game/Textures/T_Black_Mask_VT"),
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
