#include "CreadorMaterialMuroPiedra.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Engine/Texture2D.h"
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

	int32 gx = -800, gy = -200;
	auto New   = [&](UClass* C, int32 y){ gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y){ auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };
	auto Bin   = [&](UClass* C, UMaterialExpression* A, UMaterialExpression* B, int32 y)
	{ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(A, TEXT(""), e, TEXT("A")); ML::ConnectMaterialExpressions(B, TEXT(""), e, TEXT("B")); return e; };
	auto Mul = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionMultiply::StaticClass(), A, B, y); };

	// World pos -> UV (1m tiling)
	auto* WP = New(UMaterialExpressionWorldPosition::StaticClass(), -200);
	auto* mX = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -240));
	mX->R = true; mX->G = false; mX->B = false; mX->A = false;
	auto* mY = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -160));
	mY->R = false; mY->G = true; mY->B = false; mY->A = false;
	ML::ConnectMaterialExpressions(WP, TEXT(""), mX, TEXT(""));
	ML::ConnectMaterialExpressions(WP, TEXT(""), mY, TEXT(""));

	auto* u = Mul(mX, Const(1.f / 100.f, -160), -150);
	auto* v = Mul(mY, Const(1.f / 100.f, -240), -250);
	auto* uv = Bin(UMaterialExpressionAppendVector::StaticClass(), u, v, -200);

	// Color texture
	auto* tex = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 0));
	tex->ParameterName = TEXT("BaseTexture");
	if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Textures/T_StoneWall_Color.StoneWall_Color")))
		tex->Texture = T;
	else
		tex->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	ML::ConnectMaterialExpressions(uv, TEXT(""), tex, TEXT("UVs"));
	ML::ConnectMaterialProperty(tex, TEXT("RGB"), MP_BaseColor);

	// Normal map
	auto* nrm = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 100));
	nrm->ParameterName = TEXT("NormalTex");
	nrm->SamplerType = SAMPLERTYPE_Normal;
	if (UTexture2D* N = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Textures/T_StoneWall_Normal.StoneWall_Normal")))
		nrm->Texture = N;
	else
		nrm->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture"));
	ML::ConnectMaterialExpressions(uv, TEXT(""), nrm, TEXT("UVs"));
	ML::ConnectMaterialProperty(nrm, TEXT("RGB"), MP_Normal);

	// Roughness
	UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
	UMaterialExpression* rough = Const(0.8f, 200);
	if (MPC)
	{
		auto* wet = Cast<UMaterialExpressionCollectionParameter>(New(UMaterialExpressionCollectionParameter::StaticClass(), 240));
		wet->Collection = MPC; wet->ParameterName = TEXT("Wetness");
		auto* lr = Cast<UMaterialExpressionLinearInterpolate>(New(UMaterialExpressionLinearInterpolate::StaticClass(), 280));
		ML::ConnectMaterialExpressions(Const(0.8f, 260), TEXT(""), lr, TEXT("A"));
		ML::ConnectMaterialExpressions(Const(0.3f, 300), TEXT(""), lr, TEXT("B"));
		ML::ConnectMaterialExpressions(wet, TEXT(""), lr, TEXT("Alpha"));
		rough = lr;
	}
	ML::ConnectMaterialProperty(rough, TEXT(""), MP_Roughness);

	// AO
	auto* ao = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 340));
	ao->ParameterName = TEXT("AOTex");
	ao->SamplerType = SAMPLERTYPE_Normal;
	if (UTexture2D* A = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Textures/T_StoneWall_AO.StoneWall_AO")))
		ao->Texture = A;
	else
		ao->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture"));
	ML::ConnectMaterialExpressions(uv, TEXT(""), ao, TEXT("UVs"));
	ML::ConnectMaterialProperty(ao, TEXT("R"), MP_AmbientOcclusion);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[MuroPiedra] material creado en %s"), *Ruta);
	return true;
}
