#include "CreadorMaterialTejas.h"
#include "CreadorPBRComun.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

bool UCreadorMaterialTejas::CrearMaterialTejas()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre = TEXT("M_Techo_Tejas");
	const FString Ruta = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Tejas] no pude crear el material")); return false; }

	// Teja curva de 30 cm. La terracota vieja tiene líquenes y desgaste: eso
	// está en el mapa de roughness y en el AO del set, no en una constante.
	AlsasuaPBR::FOpciones Op;
	Op.Set = TEXT("RoofTiles");
	Op.TileCm = 30.f;
	Op.RoughnessMojado = 0.15f;
	AlsasuaPBR::Cablear(Mat, Op);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[Tejas] material creado en %s"), *Ruta);
	return true;
}
