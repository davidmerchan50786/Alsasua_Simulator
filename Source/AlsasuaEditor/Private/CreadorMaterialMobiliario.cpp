#include "CreadorMaterialMobiliario.h"
#include "CreadorPBRComun.h"
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

	// El color por vértice distingue cada pieza (banco, papelera, bolardo) y
	// hace de tinte sobre la veta de madera del set: T_Wood_* estaba descargado
	// y sin usar, y el mobiliario salía en color liso sin grano ni relieve.
	auto* VC = New(UMaterialExpressionVertexColor::StaticClass(), -100);

	AlsasuaPBR::FOpciones Op;
	Op.Set = TEXT("Wood");
	Op.TileCm = 60.f;
	Op.RoughnessMojado = 0.2f;
	Op.Tinte = VC;
	if (!AlsasuaPBR::Cablear(Mat, Op))
	{
		ML::ConnectMaterialProperty(VC, TEXT(""), MP_BaseColor);

		UMaterialExpression* roughness = Const(0.65f, 100);
		if (UMaterialExpression* wet = AlsasuaPBR::Clima(Mat, TEXT("Wetness"), -700, 140))
		{
			auto* lr = Cast<UMaterialExpressionLinearInterpolate>(New(UMaterialExpressionLinearInterpolate::StaticClass(), 180));
			ML::ConnectMaterialExpressions(Const(0.65f, 160), TEXT(""), lr, TEXT("A"));
			ML::ConnectMaterialExpressions(Const(0.15f, 200), TEXT(""), lr, TEXT("B"));
			ML::ConnectMaterialExpressions(wet, TEXT(""), lr, TEXT("Alpha"));
			roughness = lr;
		}
		ML::ConnectMaterialProperty(roughness, TEXT(""), MP_Roughness);
	}

	// No metálico: aquí manda la madera. El hierro va en M_Metal.
	ML::ConnectMaterialProperty(Const(0.f, 240), TEXT(""), MP_Metallic);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[Mobiliario] material creado en %s"), *Ruta);
	return true;
}

bool UCreadorMaterialMobiliario::CrearMaterialMetal()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre = TEXT("M_Metal");
	const FString Ruta = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Metal] no pude crear el material")); return false; }

	int32 gx = -700, gy = -100;
	auto New   = [&](UClass* C, int32 y){ gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y){ auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };

	// Hierro pintado de farolas, barandillas y semáforos: el color por vértice
	// da el verde oscuro o el negro de cada pieza y T_MetalPlate_* aporta la
	// chapa, sus abolladuras y el desgaste de la pintura.
	auto* VC = New(UMaterialExpressionVertexColor::StaticClass(), -100);

	AlsasuaPBR::FOpciones Op;
	Op.Set = TEXT("MetalPlate");
	Op.TileCm = 40.f;
	Op.EscalaRoughness = 0.8f;    // pintura semimate, no chapa desnuda
	Op.RoughnessMojado = 0.1f;
	Op.Tinte = VC;
	if (!AlsasuaPBR::Cablear(Mat, Op))
	{
		ML::ConnectMaterialProperty(VC, TEXT(""), MP_BaseColor);
		ML::ConnectMaterialProperty(Const(0.4f, 100), TEXT(""), MP_Roughness);
	}

	// Pintado sobre metal: el especular es dieléctrico, no metálico puro. Con
	// Metallic 1 la pintura perdería su color bajo Lumen.
	ML::ConnectMaterialProperty(Const(0.25f, 240), TEXT(""), MP_Metallic);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[Metal] material creado en %s"), *Ruta);
	return true;
}
