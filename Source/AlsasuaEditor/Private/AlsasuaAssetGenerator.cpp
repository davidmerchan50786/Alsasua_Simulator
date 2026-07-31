// AlsasuaAssetGenerator.cpp — stubbed, UE5.4 API incompatibilities
// TODO: fix FStaticMeshAttributes, UTextureFactory, UAssetImportTask APIs for UE5.4
#if WITH_EDITOR
#include "AlsasuaAssetGenerator.h"
#include "CoreMinimal.h"

bool UAlsasuaAssetGenerator::GenerarTodosLosAssets() { UE_LOG(LogTemp, Warning, TEXT("AssetGenerator stubbed — needs UE5.4 API update")); return false; }
bool UAlsasuaAssetGenerator::ImportarOrtofoto() { return false; }
bool UAlsasuaAssetGenerator::CrearTodosLosMateriales() { return false; }
bool UAlsasuaAssetGenerator::GenerarMeshesArboles() { return false; }
bool UAlsasuaAssetGenerator::GenerarMobiliarioUrbano() { return false; }
bool UAlsasuaAssetGenerator::GenerarLandmarks() { return false; }
bool UAlsasuaAssetGenerator::GenerarRios() { return false; }
bool UAlsasuaAssetGenerator::GenerarPuentes() { return false; }
bool UAlsasuaAssetGenerator::ScanFoliage() { return false; }
bool UAlsasuaAssetGenerator::CrearMaterialesPBR() { return false; }
void UAlsasuaAssetGenerator::CrearCarpeta(const FString&) {}
#endif
