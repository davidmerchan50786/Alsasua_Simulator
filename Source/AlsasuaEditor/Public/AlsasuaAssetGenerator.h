// AlsasuaAssetGenerator.h (sólo editor)
// Generador maestro: ejecuta todas las herramientas de creación de assets visuales
// en el orden correcto. Genera materiales, importa la ortofoto, crea meshes de árboles
// y mobiliario urbano, y coloca modelos de landmarks.
#pragma once

#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "AlsasuaAssetGenerator.generated.h"

UCLASS()
class ALSASUAEDITOR_API UAlsasuaAssetGenerator : public UBlueprintFunctionLibrary
{
	GENERATED_BODY()

public:
	/** Ejecuta todo el pipeline de generación de assets visuales. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool GenerarTodosLosAssets();

	/** Paso 1: Importar ortofoto PNOA como T_Ortofoto. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool ImportarOrtofoto();

	/** Paso 2: Crear todos los materiales (6 materiales + MPC). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool CrearTodosLosMateriales();

	/** Paso 3: Generar meshes de árboles procedurales (10 especies). */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool GenerarMeshesArboles();

	/** Paso 4: Generar meshes de mobiliario urbano. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool GenerarMobiliarioUrbano();

	/** Paso 5: Generar modelos de landmarks de Alsasua. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool GenerarLandmarks();

	/** Paso 6: Generar lechos de río y bancas de ribera. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool GenerarRios();

	/** Paso 7: Generar puentes de piedra y hierro. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool GenerarPuentes();

	/** Paso 8: Scan Fab Megascans foliage. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool ScanFoliage();

	/** Paso 9: Crear materiales de calles, acera, muro piedra, tejas. */
	UFUNCTION(BlueprintCallable, CallInEditor, Category="Alsasua|AssetGenerator")
	static bool CrearMaterialesPBR();

private:
	static void CrearCarpeta(const FString& Ruta);
};
