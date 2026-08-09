// AlsasuaAssetGenerator.cpp — orquestador del pipeline de assets visuales.
//
// El orden no es decorativo: hay dependencias reales entre pasos.
//   1. La ortofoto y las texturas PNG tienen que existir como assets antes de
//      que cualquier material las muestree; si no, LoadObject devuelve null y
//      el material se queda con la textura por defecto (gris).
//   2. M_Edificio es quien crea MPC_Clima (AsegurarMPCClima). Todos los demás
//      materiales leen de él su parámetro Wetness, así que va primero de los
//      materiales o el resto sale sin respuesta a la lluvia.
//   3. Los generadores de geometría (ríos, puentes) asignan materiales al
//      crearse, así que van después de los materiales.
//   4. El escaneo de Fab va al final: sustituye lo procedural por lo bajado.
#if WITH_EDITOR
#include "AlsasuaAssetGenerator.h"
#include "CoreMinimal.h"
#include "CreadorMaterialEdificio.h"
#include "CreadorMaterialFachada.h"
#include "CreadorMaterialCalles.h"
#include "CreadorMaterialTejas.h"
#include "CreadorMaterialMuroPiedra.h"
#include "CreadorMaterialMobiliario.h"
#include "CreadorMaterialArbol.h"
#include "CreadorMaterialAgua.h"
#include "CreadorMaterialTerrenoOrto.h"
#include "CreadorMaterialTejadoOrto.h"
#include "AlsasuaRiverMeshGenerator.h"
#include "AlsasuaBridgeGenerator.h"
#include "AlsasuaFoliageLoader.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "AssetImportTask.h"
#include "EditorAssetLibrary.h"
#include "Engine/Texture2D.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Modules/ModuleManager.h"

namespace
{
	/** Un paso del pipeline, para poder informar de qué falló sin abortar todo. */
	struct FPaso
	{
		const TCHAR* Nombre;
		bool (*Ejecutar)();
	};

	int32 EjecutarPasos(const TCHAR* Fase, const TArray<FPaso>& Pasos)
	{
		int32 Ok = 0;
		for (const FPaso& P : Pasos)
		{
			const bool bOk = P.Ejecutar && P.Ejecutar();
			if (bOk)
			{
				++Ok;
				UE_LOG(LogTemp, Log, TEXT("[Assets]   OK    %s"), P.Nombre);
			}
			else
			{
				UE_LOG(LogTemp, Warning, TEXT("[Assets]   FALLO %s"), P.Nombre);
			}
		}
		UE_LOG(LogTemp, Log, TEXT("[Assets] %s: %d/%d"), Fase, Ok, Pasos.Num());
		return Ok;
	}

	/** Importa un PNG de Content/Textures como UTexture2D con ajustes de color. */
	bool ImportarPNG(const FString& NombreFichero, const FString& NombreAsset)
	{
		const FString Destino = TEXT("/Game/Textures");
		if (UEditorAssetLibrary::DoesAssetExist(Destino / NombreAsset))
		{
			UE_LOG(LogTemp, Log, TEXT("[Assets] %s ya importada"), *NombreAsset);
			return true;
		}

		const FString Origen = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Textures"), NombreFichero);
		if (!FPaths::FileExists(Origen))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Assets] no existe %s"), *Origen);
			return false;
		}

		UAssetImportTask* Tarea = NewObject<UAssetImportTask>();
		Tarea->Filename = Origen;
		Tarea->DestinationPath = Destino;
		Tarea->DestinationName = NombreAsset;
		Tarea->bAutomated = true;
		Tarea->bSave = true;
		Tarea->bReplaceExisting = false;

		IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		TArray<UAssetImportTask*> Tareas{ Tarea };
		AT.ImportAssetTasks(Tareas);

		return UEditorAssetLibrary::DoesAssetExist(Destino / NombreAsset);
	}
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paso 1: ortofoto PNOA
// ─────────────────────────────────────────────────────────────────────────────
bool UAlsasuaAssetGenerator::ImportarOrtofoto()
{
	// El resto de Content/Textures lo auto-importa el editor al arrancar; la
	// ortofoto necesita nombre concreto porque M_Terreno_Orto y M_Tejado_Orto
	// la buscan como /Game/Textures/T_Ortofoto.
	return ImportarPNG(TEXT("ortofoto_pnoa_plaza_8192.png"), TEXT("T_Ortofoto"));
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paso 2: materiales base. M_Edificio primero: crea MPC_Clima.
// ─────────────────────────────────────────────────────────────────────────────
bool UAlsasuaAssetGenerator::CrearTodosLosMateriales()
{
	// MPC_Clima antes que nada: sin él ningún material responde a la lluvia.
	if (!UCreadorMaterialEdificio::CrearMaterialEdificio())
	{
		UE_LOG(LogTemp, Error, TEXT("[Assets] M_Edificio/MPC_Clima falló: el resto saldría sin Wetness. Aborto la fase."));
		return false;
	}

	const TArray<FPaso> Pasos = {
		{ TEXT("M_Fachada"),        &UCreadorMaterialFachada::CrearMaterialFachada },
		{ TEXT("M_Arbol"),          &UCreadorMaterialArbol::CrearMaterialArbol },
		{ TEXT("M_Agua"),           &UCreadorMaterialAgua::CrearMaterialAgua },
		{ TEXT("M_Terreno_Orto"),   &UCreadorMaterialTerrenoOrto::CrearMaterialTerrenoOrto },
		{ TEXT("M_Tejado_Orto"),    &UCreadorMaterialTejadoOrto::CrearMaterialTejadoOrto },
	};
	return EjecutarPasos(TEXT("Materiales base"), Pasos) == Pasos.Num();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Paso 9 (antes que la geometría): superficies PBR de Content/Textures
// ─────────────────────────────────────────────────────────────────────────────
bool UAlsasuaAssetGenerator::CrearMaterialesPBR()
{
	const TArray<FPaso> Pasos = {
		{ TEXT("M_Terreno_Calles"), &UCreadorMaterialCalles::CrearMaterialCalles },
		{ TEXT("M_Terreno_Acera"),  &UCreadorMaterialCalles::CrearMaterialAcera },
		{ TEXT("M_Terreno_Hierba"), &UCreadorMaterialCalles::CrearMaterialHierba },
		{ TEXT("M_Terreno_Tierra"), &UCreadorMaterialCalles::CrearMaterialTierra },
		{ TEXT("M_Marca_Blanca"),   &UCreadorMaterialCalles::CrearMaterialMarcaBlanca },
		{ TEXT("M_Techo_Tejas"),    &UCreadorMaterialTejas::CrearMaterialTejas },
		{ TEXT("M_Muro_Piedra"),    &UCreadorMaterialMuroPiedra::CrearMaterialMuroPiedra },
		{ TEXT("M_Mobiliario"),     &UCreadorMaterialMobiliario::CrearMaterialMobiliario },
		{ TEXT("M_Metal"),          &UCreadorMaterialMobiliario::CrearMaterialMetal },
	};
	return EjecutarPasos(TEXT("Superficies PBR"), Pasos) == Pasos.Num();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pasos de geometría: van después de los materiales, que es lo que asignan.
// ─────────────────────────────────────────────────────────────────────────────
bool UAlsasuaAssetGenerator::GenerarRios()
{
	const TArray<FPaso> Pasos = {
		{ TEXT("Lecho de río"),  &UAlsasuaRiverMeshGenerator::GenerarLechoRio },
		{ TEXT("Bancas ribera"), &UAlsasuaRiverMeshGenerator::GenerarBancasRio },
	};
	return EjecutarPasos(TEXT("Ríos"), Pasos) == Pasos.Num();
}

bool UAlsasuaAssetGenerator::GenerarPuentes()
{
	return UAlsasuaBridgeGenerator::GenerarPuentesMejorados();
}

bool UAlsasuaAssetGenerator::ScanFoliage()
{
	return UAlsasuaFoliageLoader::ScanAndRegisterFoliage();
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pasos que siguen necesitando trabajo de API de malla (no los invento).
// ─────────────────────────────────────────────────────────────────────────────
bool UAlsasuaAssetGenerator::GenerarMeshesArboles()
{
	UE_LOG(LogTemp, Warning, TEXT("[Assets] Árboles procedurales: sin implementar (necesita FStaticMeshAttributes). Usa el paso de Fab."));
	return false;
}

bool UAlsasuaAssetGenerator::GenerarMobiliarioUrbano()
{
	UE_LOG(LogTemp, Warning, TEXT("[Assets] Mobiliario procedural: sin implementar (necesita FStaticMeshAttributes)."));
	return false;
}

bool UAlsasuaAssetGenerator::GenerarLandmarks()
{
	UE_LOG(LogTemp, Warning, TEXT("[Assets] Landmarks: sin implementar (necesita FStaticMeshAttributes)."));
	return false;
}

// ─────────────────────────────────────────────────────────────────────────────
//  Pipeline completo, en orden de dependencias.
// ─────────────────────────────────────────────────────────────────────────────
bool UAlsasuaAssetGenerator::GenerarTodosLosAssets()
{
	UE_LOG(LogTemp, Log, TEXT("[Assets] ================ Pipeline de assets ================"));

	CrearCarpeta(TEXT("/Game/Materiales"));
	CrearCarpeta(TEXT("/Game/Textures"));

	// 1. Texturas primero: los materiales las muestrean.
	ImportarOrtofoto();

	// 2. MPC_Clima + materiales base.
	const bool bBase = CrearTodosLosMateriales();

	// 3. Superficies PBR (dependen de MPC_Clima del paso anterior).
	const bool bPBR = CrearMaterialesPBR();

	// 4. Geometría, que asigna esos materiales.
	const bool bRios = GenerarRios();
	const bool bPuentes = GenerarPuentes();

	// 5. Fab al final: lo bajado sustituye a lo procedural.
	const bool bFoliage = ScanFoliage();

	UE_LOG(LogTemp, Log, TEXT("[Assets] ---------------- Resumen ----------------"));
	UE_LOG(LogTemp, Log, TEXT("[Assets]  Materiales base : %s"), bBase    ? TEXT("OK") : TEXT("con fallos"));
	UE_LOG(LogTemp, Log, TEXT("[Assets]  Superficies PBR : %s"), bPBR     ? TEXT("OK") : TEXT("con fallos"));
	UE_LOG(LogTemp, Log, TEXT("[Assets]  Ríos            : %s"), bRios    ? TEXT("OK") : TEXT("con fallos"));
	UE_LOG(LogTemp, Log, TEXT("[Assets]  Puentes         : %s"), bPuentes ? TEXT("OK") : TEXT("con fallos"));
	UE_LOG(LogTemp, Log, TEXT("[Assets]  Foliage de Fab  : %s"), bFoliage ? TEXT("OK") : TEXT("nada bajado"));
	UE_LOG(LogTemp, Log, TEXT("[Assets] Pendiente a mano: árboles, mobiliario y landmarks procedurales."));
	UE_LOG(LogTemp, Log, TEXT("[Assets] ====================================================="));

	// Materiales y superficies son lo imprescindible para que el pueblo no
	// salga gris; el resto es contenido añadido.
	return bBase && bPBR;
}

void UAlsasuaAssetGenerator::CrearCarpeta(const FString& Ruta)
{
	if (!UEditorAssetLibrary::DoesDirectoryExist(Ruta))
	{
		UEditorAssetLibrary::MakeDirectory(Ruta);
	}
}
#endif
