// CreadorMaterialTejadoOrto.cpp (sólo editor)
#include "CreadorMaterialTejadoOrto.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Engine/Texture2D.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

// Límites del orto PNOA urbano (2750x2750 m centrado en la plaza) en mundo Unreal (cm).
// world_cm = (UTM_m - 566033, UTM_m - 4741332)*100 ; textura SUR arriba
// (v=0 en Ymin). u=(wX-Xmin)/rango, v=(wY-Ymin)/rango.
namespace {
static const float TOWN_XMIN_CM = 54300.f, TOWN_RANGO_CM = 275000.f;
static const float TOWN_YMIN_CM = 719500.f;
}

bool UCreadorMaterialTejadoOrto::CrearMaterialTejadoOrto()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Ruta    = Carpeta / TEXT("M_Tejado_Orto");

	if (UEditorAssetLibrary::DoesAssetExist(Ruta)) UEditorAssetLibrary::DeleteAsset(Ruta);
	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(TEXT("M_Tejado_Orto"), Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[TejadoOrto] no pude crear el material")); return false; }

	int32 gx = -900, gy = -200;
	auto New   = [&](UClass* C, int32 y){ gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y){ auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };
	auto Bin   = [&](UClass* C, UMaterialExpression* A, UMaterialExpression* B, int32 y){ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(A, TEXT(""), e, TEXT("A")); ML::ConnectMaterialExpressions(B, TEXT(""), e, TEXT("B")); return e; };
	auto Un    = [&](UClass* C, UMaterialExpression* X, int32 y){ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(X, TEXT(""), e, TEXT("")); return e; };
	auto Mul   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionMultiply::StaticClass(), A, B, y); };
	auto Sub   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionSubtract::StaticClass(), A, B, y); };
	auto Add   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionAdd::StaticClass(), A, B, y); };
	auto Sat   = [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionSaturate::StaticClass(), X, y); };

	// WorldPosition -> X, Y
	auto* WP = New(UMaterialExpressionWorldPosition::StaticClass(), -200);
	auto* wX = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -240)); wX->R=true;  wX->G=false; wX->B=false; wX->A=false;
	auto* wY = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -160)); wY->R=false; wY->G=true;  wY->B=false; wY->A=false;
	ML::ConnectMaterialExpressions(WP, TEXT(""), wX, TEXT(""));
	ML::ConnectMaterialExpressions(WP, TEXT(""), wY, TEXT(""));

	// texU = (WPos.X - TOWN_XMIN) / RANGO ; texV = (WPos.Y - TOWN_YMIN) / RANGO
	auto* u = Mul(Sub(wX, Const(TOWN_XMIN_CM, -160), -160), Const(1.f / TOWN_RANGO_CM, -160), -150);
	auto* v = Mul(Sub(wY, Const(TOWN_YMIN_CM, -240), -240), Const(1.f / TOWN_RANGO_CM, -240), -250);
	auto* uv = Bin(UMaterialExpressionAppendVector::StaticClass(), u, v, -200);   // float2(U,V)

	// Muestra de la ortofoto (parámetro asignable).
	auto* tex = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 0));
	tex->ParameterName = TEXT("Ortofoto");
	if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Textures/T_Ortofoto.T_Ortofoto")))
		tex->Texture = T;
	else
	{
		tex->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
		UE_LOG(LogTemp, Warning, TEXT("[TejadoOrto] importa /Game/Textures/T_Ortofoto y asigna el parametro 'Ortofoto'."));
	}
	ML::ConnectMaterialExpressions(uv, TEXT(""), tex, TEXT("UVs"));

	// --- Detalle de teja de cerca: textura tileada que modula el brillo, fundida por distancia ---
	auto* dUV = Bin(UMaterialExpressionAppendVector::StaticClass(),
		Mul(wX, Const(1.f / 100.f, 300), 300), Mul(wY, Const(1.f / 100.f, 360), 360), 330);   // tile 1 m
	auto* det = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 420));
	det->ParameterName = TEXT("Detalle");
	det->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	ML::ConnectMaterialExpressions(dUV, TEXT(""), det, TEXT("UVs"));
	auto* dLum = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), 480)); dLum->R=true; dLum->G=false; dLum->B=false; dLum->A=false;
	ML::ConnectMaterialExpressions(det, TEXT("RGB"), dLum, TEXT(""));
	auto* centrado = Sub(dLum, Const(0.5f, 480), 480);

	auto* depth = New(UMaterialExpressionPixelDepth::StaticClass(), 540);
	auto* fade  = Sat(Mul(Sub(Const(5000.f, 540), depth, 540), Const(1.f / 3500.f, 540), 540), 540);
	auto* factor = Add(Const(1.f, 400), Mul(centrado, fade, 440), 420);

	ML::ConnectMaterialProperty(Mul(tex, factor, 0), TEXT(""), MP_BaseColor);   // ortofoto * detalle

	// Relieve de teja de cerca: normal map tileado, fundido a plano por distancia.
	auto* plano = Cast<UMaterialExpressionConstant3Vector>(New(UMaterialExpressionConstant3Vector::StaticClass(), 600)); plano->Constant = FLinearColor(0, 0, 1);
	ML::ConnectMaterialProperty(plano, TEXT(""), MP_Normal);

	auto* rough = Const(0.7f, 200);
	ML::ConnectMaterialProperty(rough, TEXT(""), MP_Roughness);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[TejadoOrto] material creado en %s"), *Ruta);
	return true;
}
