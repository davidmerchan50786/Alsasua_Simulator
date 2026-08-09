#include "AlsasuaFoliageLoader.h"
#include "EditorAssetLibrary.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "AssetRegistry/AssetRegistryModule.h"
#include "AssetRegistry/ARFilter.h"
#include "Modules/ModuleManager.h"

/**
 * Especies de Alsasua y con qué nombres aparecen al bajarlas.
 *
 * Fab/Quixel no importa con el nombre latino ni con una ruta fija: cada asset
 * cae en su propia carpeta con un hash, del estilo
 * /Game/Megascans/3D_Plants/European_Linden_vjxebfy/SM_vjxebfy_Var1_lod0.
 * Por eso hay que buscar por palabra clave en el registro de assets: los
 * nombres en latín y los que usa Megascans en inglés.
 */
namespace
{
	struct FEspecie
	{
		const TCHAR* Latin;
		const TCHAR* Claves;   // separadas por '|', en minúsculas
	};

	const FEspecie Especies[] = {
		{ TEXT("Tilia"),        TEXT("tilia|linden|lime_tree|basswood") },
		{ TEXT("Platanus"),     TEXT("platanus|plane_tree|planetree|sycamore") },
		{ TEXT("QuercusRobur"), TEXT("quercus|oak") },
		{ TEXT("Pinus"),        TEXT("pinus|pine") },
		{ TEXT("Fagus"),        TEXT("fagus|beech") },
		{ TEXT("Betula"),       TEXT("betula|birch") },
		{ TEXT("Populus"),      TEXT("populus|poplar|aspen|cottonwood") },
		{ TEXT("Salix"),        TEXT("salix|willow") },
		{ TEXT("Prunus"),       TEXT("prunus|cherry|blackthorn") },
		{ TEXT("Acer"),         TEXT("acer|maple") },
	};

	/** Carpetas donde aterrizan las descargas de Fab y de la biblioteca de Epic. */
	const TCHAR* RaicesFab[] = {
		TEXT("/Game/Megascans"),
		TEXT("/Game/MSPresets"),
		TEXT("/Game/Fab"),
	};

	/** Todos los UStaticMesh bajo las raíces de Fab; si no hay ninguna, bajo /Game. */
	void RecogerMallas(TArray<FAssetData>& Out)
	{
		FAssetRegistryModule& ARM = FModuleManager::LoadModuleChecked<FAssetRegistryModule>(TEXT("AssetRegistry"));
		IAssetRegistry& AR = ARM.Get();

		TArray<FString> Raices;
		for (const TCHAR* Raiz : RaicesFab)
		{
			if (UEditorAssetLibrary::DoesDirectoryExist(Raiz))
			{
				Raices.Add(Raiz);
			}
		}

		// Nada importado todavía: no barremos /Game entero para nada.
		if (Raices.Num() == 0) return;

		AR.ScanPathsSynchronous(Raices, /*bForceRescan*/ false);

		FARFilter Filtro;
		Filtro.ClassPaths.Add(UStaticMesh::StaticClass()->GetClassPathName());
		Filtro.bRecursivePaths = true;
		Filtro.bRecursiveClasses = true;
		for (const FString& Raiz : Raices)
		{
			Filtro.PackagePaths.Add(FName(*Raiz));
		}

		AR.GetAssets(Filtro, Out);
	}

	/** ¿El nombre del asset contiene alguna de las claves de la especie? */
	bool Coincide(const FString& NombreMinusculas, const FString& Claves)
	{
		TArray<FString> Lista;
		Claves.ParseIntoArray(Lista, TEXT("|"), true);
		for (const FString& Clave : Lista)
		{
			if (NombreMinusculas.Contains(Clave)) return true;
		}
		return false;
	}

	/**
	 * Entre varias variantes de la misma especie preferimos el LOD más detallado:
	 * Megascans exporta SM_..._lod0..lod7 y las últimas son cortezas sin hoja.
	 */
	int32 Prioridad(const FString& NombreMinusculas)
	{
		if (NombreMinusculas.Contains(TEXT("lod0"))) return 0;
		if (!NombreMinusculas.Contains(TEXT("lod"))) return 1;
		return 2;
	}
}

bool UAlsasuaFoliageLoader::ScanAndRegisterFoliage()
{
	TArray<FAssetData> Mallas;
	RecogerMallas(Mallas);

	if (Mallas.Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Foliage] No hay contenido de Fab en /Game/Megascans, /Game/MSPresets ni /Game/Fab."));
		UE_LOG(LogTemp, Log, TEXT("[Foliage] Ventana → Fab, inicia sesión con tu cuenta de Epic y descarga los árboles ('Add to Project')."));
		UE_LOG(LogTemp, Log, TEXT("[Foliage] Para Alsasua: tilo, plátano de sombra, roble, haya, abedul, chopo, sauce, cerezo y arce."));
		return false;
	}

	TMap<FString, FString> Encontradas;
	TMap<FString, int32> MejorPrioridad;

	for (const FAssetData& Malla : Mallas)
	{
		const FString Nombre = Malla.AssetName.ToString().ToLower();
		const FString RutaPaquete = Malla.PackageName.ToString().ToLower();

		for (const FEspecie& Esp : Especies)
		{
			// La especie puede estar en el nombre del asset o en su carpeta:
			// Megascans deja el nombre legible en la carpeta y un hash en el mesh.
			if (!Coincide(Nombre, Esp.Claves) && !Coincide(RutaPaquete, Esp.Claves)) continue;

			const int32 P = Prioridad(Nombre);
			const int32* Actual = MejorPrioridad.Find(Esp.Latin);
			if (!Actual || P < *Actual)
			{
				MejorPrioridad.Add(Esp.Latin, P);
				Encontradas.Add(Esp.Latin, Malla.GetSoftObjectPath().ToString());
			}
			break;
		}
	}

	for (const FEspecie& Esp : Especies)
	{
		if (const FString* Ruta = Encontradas.Find(Esp.Latin))
		{
			UE_LOG(LogTemp, Log, TEXT("[Foliage] %s -> %s"), Esp.Latin, **Ruta);
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Foliage] %s sin malla en Fab (se queda con el árbol procedural)."), Esp.Latin);
		}
	}

	// El mapeo se guarda para que el poblado del mundo lo lea sin re-escanear.
	if (Encontradas.Num() > 0)
	{
		FString Json;
		TSharedRef<TJsonWriter<>> W = TJsonWriterFactory<>::Create(&Json);
		W->WriteObjectStart();
		for (const TPair<FString, FString>& Par : Encontradas)
		{
			W->WriteValue(Par.Key, Par.Value);
		}
		W->WriteObjectEnd();
		W->Close();

		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/fab_foliage.json"));
		if (FFileHelper::SaveStringToFile(Json, *Ruta))
		{
			UE_LOG(LogTemp, Log, TEXT("[Foliage] Mapeo guardado en Datos/fab_foliage.json"));
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Foliage] %d de %d especies cubiertas por Fab (%d mallas revisadas)."),
		Encontradas.Num(), (int32)UE_ARRAY_COUNT(Especies), Mallas.Num());

	return Encontradas.Num() > 0;
}

bool UAlsasuaFoliageLoader::ReplaceProceduralTreesWithFoliage()
{
	// El escaneo deja el mapeo especie -> malla de Fab en Datos/fab_foliage.json.
	if (!ScanAndRegisterFoliage()) return false;

	UE_LOG(LogTemp, Log, TEXT("[Foliage] Pinta las especies con la herramienta Foliage usando las mallas del mapeo,"));
	UE_LOG(LogTemp, Log, TEXT("[Foliage] o arrástralas al nivel: sustituyen a los árboles procedurales de la misma especie."));
	return true;
}
