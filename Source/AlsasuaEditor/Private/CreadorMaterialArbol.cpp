// CreadorMaterialArbol.cpp (sólo editor)
#include "CreadorMaterialArbol.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVectorParameter.h"
#include "Materials/MaterialExpressionConstant.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

bool UCreadorMaterialArbol::CrearMaterialArbol()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre  = TEXT("M_Arbol");
	const FString Ruta    = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UObject* Obj = AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>());
	UMaterial* Mat = Cast<UMaterial>(Obj);
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Arbol] no pude crear el material")); return false; }

	// Parámetro vectorial "Color" -> Base Color (lo fija la instancia por especie).
	UMaterialExpressionVectorParameter* Col = Cast<UMaterialExpressionVectorParameter>(
		UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionVectorParameter::StaticClass(), -500, -100));
	Col->ParameterName = TEXT("Color");
	Col->DefaultValue  = FLinearColor(0.24f, 0.40f, 0.16f);
	UMaterialEditingLibrary::ConnectMaterialProperty(Col, TEXT(""), MP_BaseColor);

	// Follaje: rugoso, sin metal.
	UMaterialExpressionConstant* Rough = Cast<UMaterialExpressionConstant>(
		UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -500, 80));
	Rough->R = 0.62f;
	UMaterialEditingLibrary::ConnectMaterialProperty(Rough, TEXT(""), MP_Roughness);

	Mat->PostEditChange();
	UMaterialEditingLibrary::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);

	UE_LOG(LogTemp, Log, TEXT("[Arbol] material creado en %s"), *Ruta);
	return true;
}
