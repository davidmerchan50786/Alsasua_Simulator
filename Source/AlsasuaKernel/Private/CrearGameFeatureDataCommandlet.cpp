#include "CrearGameFeatureDataCommandlet.h"
#include "GameFeatureData.h"
#include "Interfaces/IPluginManager.h"
#include "Misc/PackageName.h"
#include "Misc/Paths.h"
#include "PackageTools.h"
#include "UObject/Package.h"
#include "UObject/SavePackage.h"

DEFINE_LOG_CATEGORY_STATIC(LogCrearGFD, Log, All);

static const TCHAR** ListaPlugins(int32& Num)
{
	static const TCHAR* Plugins[] = {
		TEXT("GF_Abilities"), TEXT("GF_AI"), TEXT("GF_Audio"),
		TEXT("GF_Carreteras"), TEXT("GF_Clima"), TEXT("GF_Core"),
		TEXT("GF_Debug"), TEXT("GF_Dialogos"), TEXT("GF_Edificios"),
		TEXT("GF_Ferrocarril"), TEXT("GF_NPCs"),
		TEXT("GF_Optimization"), TEXT("GF_Politica"), TEXT("GF_Social"),
		TEXT("GF_Systems"), TEXT("GF_Trafico"), TEXT("GF_UI"),
		TEXT("GF_Vehiculos"), TEXT("GF_Vegetacion"), TEXT("GF_World"),
	};
	Num = UE_ARRAY_COUNT(Plugins);
	return Plugins;
}

UCrearGameFeatureDataCommandlet::UCrearGameFeatureDataCommandlet()
{
	IsClient = false;
	IsServer = false;
	IsEditor = true;
	LogToConsole = true;
}

int32 UCrearGameFeatureDataCommandlet::Main(const FString& Params)
{
	int32 Num = 0;
	const TCHAR** Plugins = ListaPlugins(Num);

	int32 Creados = 0;
	for (int32 i = 0; i < Num; ++i)
	{
		const FString Nombre = Plugins[i];
		// El AssetManager registra cada GFD por nombre de asset; si todos se
		// llaman GameFeatureData chocan (ensure de ID duplicado). El motor
		// acepta el backup /<Plugin>/<Plugin>.<Plugin>, unico por plugin.
		const FString RutaPaquete = FString::Printf(TEXT("/%s/%s"), *Nombre, *Nombre);

		// Ruta por fichero, no por punto de montaje: con ExplicitlyLoaded el
		// contenido del plugin no esta montado y LongPackageNameToFilename
		// no tiene raiz para /GF_X.
		const FString Archivo = FPaths::ProjectPluginsDir() / Nombre /
			TEXT("Content") / (Nombre + FPackageName::GetAssetPackageExtension());

		if (FPaths::FileExists(Archivo))
		{
			UE_LOG(LogCrearGFD, Log, TEXT("[GFD] ya existe: %s"), *Archivo);
			continue;
		}

		UPackage* Paquete = CreatePackage(*RutaPaquete);
		UGameFeatureData* Datos = NewObject<UGameFeatureData>(
			Paquete, *Nombre, RF_Public | RF_Standalone);
		if (!Datos)
		{
			UE_LOG(LogCrearGFD, Error, TEXT("[GFD] fallo creando %s"), *RutaPaquete);
			return 1;
		}
		// Sin PKG_FilterEditorOnly: con ese flag el uasset se marca cooked y un
		// build uncooked (-game de editor) rechaza cargarlo.

		FSavePackageArgs Args;
		Args.TopLevelFlags = RF_Public | RF_Standalone;
		Args.Error = GError;
		if (!UPackage::SavePackage(Paquete, Datos, *Archivo, Args))
		{
			UE_LOG(LogCrearGFD, Error, TEXT("[GFD] fallo guardando %s"), *Archivo);
			return 1;
		}
		++Creados;
		UE_LOG(LogCrearGFD, Log, TEXT("[GFD] creado: %s -> %s"), *RutaPaquete, *Archivo);
	}

	UE_LOG(LogCrearGFD, Log, TEXT("[GFD] total creados: %d de %d"), Creados, Num);
	return 0;
}
