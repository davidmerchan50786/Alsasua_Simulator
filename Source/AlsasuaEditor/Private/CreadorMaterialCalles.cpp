#include "CreadorMaterialCalles.h"
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
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Engine/Texture2D.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

static void CrearMaterialConTextura(const FString& Nombre, const FString& TexPath,
	float TileX, float TileY, float RoughnessDefault, const FString& Categoria)
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Ruta = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[%s] no pude crear el material"), *Categoria); return; }

	int32 gx = -800, gy = -200;
	auto New   = [&](UClass* C, int32 y){ gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y){ auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };
	auto Bin   = [&](UClass* C, UMaterialExpression* A, UMaterialExpression* B, int32 y)
	{ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(A, TEXT(""), e, TEXT("A")); ML::ConnectMaterialExpressions(B, TEXT(""), e, TEXT("B")); return e; };
	auto Mul = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionMultiply::StaticClass(), A, B, y); };

	// World position XY -> UV tiling
	auto* WP = New(UMaterialExpressionWorldPosition::StaticClass(), -200);
	auto* mX = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -240));
	mX->R = true; mX->G = false; mX->B = false; mX->A = false;
	auto* mY = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -160));
	mY->R = false; mY->G = true; mY->B = false; mY->A = false;
	ML::ConnectMaterialExpressions(WP, TEXT(""), mX, TEXT(""));
	ML::ConnectMaterialExpressions(WP, TEXT(""), mY, TEXT(""));

	auto* u = Mul(mX, Const(1.f / TileX, -160), -150);
	auto* v = Mul(mY, Const(1.f / TileY, -240), -250);
	auto* uv = Bin(UMaterialExpressionAppendVector::StaticClass(), u, v, -200);

	// Texture sample
	auto* tex = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 0));
	tex->ParameterName = TEXT("BaseTexture");
	if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, *TexPath))
		tex->Texture = T;
	else
		tex->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	ML::ConnectMaterialExpressions(uv, TEXT(""), tex, TEXT("UVs"));
	ML::ConnectMaterialProperty(tex, TEXT("RGB"), MP_BaseColor);

	// Wetness from MPC
	UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
	UMaterialExpression* roughnessExpr = Const(RoughnessDefault, 120);
	if (MPC)
	{
		auto* wet = Cast<UMaterialExpressionCollectionParameter>(New(UMaterialExpressionCollectionParameter::StaticClass(), 160));
		wet->Collection = MPC; wet->ParameterName = TEXT("Wetness");
		for (const FCollectionScalarParameter& P : MPC->ScalarParameters)
			if (P.ParameterName == TEXT("Wetness")) { wet->ParameterId = P.Id; break; }
		auto* lr = Cast<UMaterialExpressionLinearInterpolate>(New(UMaterialExpressionLinearInterpolate::StaticClass(), 200));
		ML::ConnectMaterialExpressions(Const(RoughnessDefault, 180), TEXT(""), lr, TEXT("A"));
		ML::ConnectMaterialExpressions(Const(RoughnessDefault * 0.2f, 220), TEXT(""), lr, TEXT("B"));
		ML::ConnectMaterialExpressions(wet, TEXT(""), lr, TEXT("Alpha"));
		roughnessExpr = lr;
	}
	ML::ConnectMaterialProperty(roughnessExpr, TEXT(""), MP_Roughness);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[%s] material creado en %s"), *Categoria, *Ruta);
}

bool UCreadorMaterialCalles::CrearMaterialCalles()
{
	// Asphalt: tiling ~2m, rough 0.7
	CrearMaterialConTextura(TEXT("M_Terreno_Calles"), TEXT("/Game/Textures/T_Asphalt_Color.T_Asphalt_Color"),
		200.f, 200.f, 0.7f, TEXT("Calles"));

	// Road detail normal
	const FString RutaCalles = TEXT("/Game/Materiales/M_Terreno_Calles");
	if (UMaterial* Mat = LoadObject<UMaterial>(nullptr, *(RutaCalles + TEXT(".M_Terreno_Calles"))))
	{
		if (UTexture2D* Nrm = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Textures/T_Asphalt_Normal.T_Asphalt_Normal")))
		{
			auto* tex = Cast<UMaterialExpressionTextureSampleParameter2D>(
				ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -800, 40));
			tex->ParameterName = TEXT("NormalTex");
			tex->SamplerType = SAMPLERTYPE_Normal;
			tex->Texture = Nrm;
			ML::ConnectMaterialProperty(tex, TEXT("RGB"), MP_Normal);
			Mat->PostEditChange();
			ML::RecompileMaterial(Mat);
			UEditorAssetLibrary::SaveAsset(RutaCalles, false);
		}
	}

	return true;
}

bool UCreadorMaterialCalles::CrearMaterialAcera()
{
	// Cobblestone: tiling ~0.5m, rough 0.85
	CrearMaterialConTextura(TEXT("M_Terreno_Acera"), TEXT("/Game/Textures/T_Cobblestone_Color.T_Cobblestone_Color"),
		50.f, 50.f, 0.85f, TEXT("Acera"));
	return true;
}

bool UCreadorMaterialCalles::CrearMaterialMarcaBlanca()
{
	const FString Nombre = TEXT("M_Marca_Blanca");
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Ruta = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[MarcaBlanca] no pude crear el material")); return false; }

	auto* Color = Cast<UMaterialExpressionConstant3Vector>(ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant3Vector::StaticClass(), 0, 0));
	Color->Constant = FLinearColor(1.f, 1.f, 1.f);
	ML::ConnectMaterialProperty(Color, TEXT("RGB"), MP_BaseColor);

	auto* Rough = Cast<UMaterialExpressionConstant>(ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), 0, 120));
	Rough->R = 0.8f;
	ML::ConnectMaterialProperty(Rough, TEXT(""), MP_Roughness);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[MarcaBlanca] material creado en %s"), *Ruta);
	return true;
}
