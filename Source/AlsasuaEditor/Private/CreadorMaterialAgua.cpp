// CreadorMaterialAgua.cpp (sólo editor)
#if WITH_EDITOR
#include "CreadorMaterialAgua.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Engine/Texture2D.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

bool UCreadorMaterialAgua::CrearMaterialAgua()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Ruta    = Carpeta / TEXT("M_AguaRio");

	if (UEditorAssetLibrary::DoesAssetExist(Ruta)) UEditorAssetLibrary::DeleteAsset(Ruta);
	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(TEXT("M_AguaRio"), Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Agua] no pude crear el material")); return false; }

	Mat->BlendMode = EBlendMode::BLEND_Translucent;
	Mat->SetShadingModel(EMaterialShadingModel::MSM_DefaultLit);
	Mat->TwoSided = true;

	using ML = UMaterialEditingLibrary;

	// Color de agua (verdeazulado del río, atenuado por translucidez).
	auto* Color = Cast<UMaterialExpressionConstant3Vector>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant3Vector::StaticClass(), -300, 0));
	Color->Constant = FLinearColor(0.10f, 0.30f, 0.35f);
	ML::ConnectMaterialProperty(Color, TEXT(""), MP_BaseColor);

	auto* Opacidad = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -300, 120));
	Opacidad->R = 0.85f;
	ML::ConnectMaterialProperty(Opacidad, TEXT(""), MP_Opacity);

	auto* Roughness = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -300, 240));
	Roughness->R = 0.15f;
	ML::ConnectMaterialProperty(Roughness, TEXT(""), MP_Roughness);

	auto* Specular = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -300, 360));
	Specular->R = 0.9f;
	ML::ConnectMaterialProperty(Specular, TEXT(""), MP_Specular);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[Agua] material creado en %s"), *Ruta);
	return true;
}
#endif
