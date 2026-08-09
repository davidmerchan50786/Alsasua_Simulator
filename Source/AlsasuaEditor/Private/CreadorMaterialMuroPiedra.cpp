#include "CreadorMaterialMuroPiedra.h"
#include "CreadorPBRComun.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

bool UCreadorMaterialMuroPiedra::CrearMaterialMuroPiedra()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre = TEXT("M_Muro_Piedra");
	const FString Ruta = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[MuroPiedra] no pude crear el material")); return false; }

	// Sillería de 1 m. El roughness sale del mapa del set: la piedra tiene
	// zonas pulidas y zonas rugosas, y con una constante 0.8 se veía uniforme.
	AlsasuaPBR::FOpciones Op;
	Op.Set = TEXT("StoneWall");
	Op.TileCm = 100.f;
	Op.RoughnessMojado = 0.3f;   // la piedra mojada refleja menos que el asfalto
	AlsasuaPBR::Cablear(Mat, Op);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[MuroPiedra] material creado en %s"), *Ruta);
	return true;
}
