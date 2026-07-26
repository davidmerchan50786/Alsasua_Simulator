#include "CreadorMaterialMobiliario.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialParameterCollection.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

bool UCreadorMaterialMobiliario::CrearMaterialMobiliario()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre = TEXT("M_Mobiliario");
	const FString Ruta = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Mobiliario] no pude crear el material")); return false; }

	int32 gx = -700, gy = -100;
	auto New   = [&](UClass* C, int32 y){ gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y){ auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };

	// Vertex color -> Base Color (so each box part gets its own color)
	auto* VC = New(UMaterialExpressionVertexColor::StaticClass(), -100);
	ML::ConnectMaterialProperty(VC, TEXT(""), MP_BaseColor);

	// Wetness from MPC
	UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
	UMaterialExpression* roughness = Const(0.65f, 100);
	if (MPC)
	{
		auto* wet = Cast<UMaterialExpressionCollectionParameter>(New(UMaterialExpressionCollectionParameter::StaticClass(), 140));
		wet->Collection = MPC; wet->ParameterName = TEXT("Wetness");
		auto* lr = Cast<UMaterialExpressionLinearInterpolate>(New(UMaterialExpressionLinearInterpolate::StaticClass(), 180));
		ML::ConnectMaterialExpressions(Const(0.65f, 160), TEXT(""), lr, TEXT("A"));
		ML::ConnectMaterialExpressions(Const(0.15f, 200), TEXT(""), lr, TEXT("B"));
		ML::ConnectMaterialExpressions(wet, TEXT(""), lr, TEXT("Alpha"));
		roughness = lr;
	}
	ML::ConnectMaterialProperty(roughness, TEXT(""), MP_Roughness);

	// Metallic = 0 (non-metal for wood/stone; can be adjusted per-instance)
	ML::ConnectMaterialProperty(Const(0.f, 240), TEXT(""), MP_Metallic);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[Mobiliario] material creado en %s"), *Ruta);
	return true;
}
