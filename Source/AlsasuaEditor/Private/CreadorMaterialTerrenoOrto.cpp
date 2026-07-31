// CreadorMaterialTerrenoOrto.cpp (sólo editor)
#include "CreadorMaterialTerrenoOrto.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Engine/Texture2D.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

// Límites del satélite PNOA completo (7200x7200 m) en mundo Unreal (cm).
// world_cm = (UTM_m - 566033, UTM_m - 4741332)*100 ; textura SUR arriba
// (v=0 en Ymin). u=(wX-Xmin)/rango, v=(wY-Ymin)/rango.
namespace {
static const float SAT_XMIN_CM = -168200.f, SAT_RANGO_CM = 720000.f;
static const float SAT_YMIN_CM = 497000.f;
}

bool UCreadorMaterialTerrenoOrto::CrearMaterialTerrenoOrto()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Ruta    = Carpeta / TEXT("M_Terreno_Orto");

	if (UEditorAssetLibrary::DoesAssetExist(Ruta)) UEditorAssetLibrary::DeleteAsset(Ruta);
	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(TEXT("M_Terreno_Orto"), Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[TerrenoOrto] no pude crear el material")); return false; }

	int32 gx = -1000, gy = -200;
	auto New   = [&](UClass* C, int32 y){ gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y){ auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };
	auto Bin   = [&](UClass* C, UMaterialExpression* A, UMaterialExpression* B, int32 y){ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(A, TEXT(""), e, TEXT("A")); ML::ConnectMaterialExpressions(B, TEXT(""), e, TEXT("B")); return e; };
	auto Un    = [&](UClass* C, UMaterialExpression* X, int32 y){ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(X, TEXT(""), e, TEXT("")); return e; };
	auto Mul   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionMultiply::StaticClass(), A, B, y); };
	auto Sub   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionSubtract::StaticClass(), A, B, y); };
	auto Add   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionAdd::StaticClass(), A, B, y); };
	auto Abs   = [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionAbs::StaticClass(), X, y); };
	auto Sat   = [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionSaturate::StaticClass(), X, y); };

	auto* WP = New(UMaterialExpressionWorldPosition::StaticClass(), -200);
	auto* wX = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -240)); wX->R=true;  wX->G=false; wX->B=false; wX->A=false;
	auto* wY = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -160)); wY->R=false; wY->G=true;  wY->B=false; wY->A=false;
	ML::ConnectMaterialExpressions(WP, TEXT(""), wX, TEXT(""));
	ML::ConnectMaterialExpressions(WP, TEXT(""), wY, TEXT(""));

	auto* u = Mul(Sub(wX, Const(SAT_XMIN_CM, -160), -160), Const(1.f / SAT_RANGO_CM, -160), -150);
	auto* v = Mul(Sub(wY, Const(SAT_YMIN_CM, -240), -240), Const(1.f / SAT_RANGO_CM, -240), -250);
	auto* uv = Bin(UMaterialExpressionAppendVector::StaticClass(), u, v, -200);

	auto* tex = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 0));
	tex->ParameterName = TEXT("Ortofoto");
	if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Terreno/T_Satelite_Alsasua.T_Satelite_Alsasua"))) tex->Texture = T;
	else tex->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	ML::ConnectMaterialExpressions(uv, TEXT(""), tex, TEXT("UVs"));

	// Máscara "dentro de la ortofoto": |U-0.5|<0.5 y |V-0.5|<0.5.
	auto* mU = Sat(Mul(Sub(Const(0.5f, 120), Abs(Sub(u, Const(0.5f, 120), 120), 120), 120), Const(60.f, 120), 120), 120);
	auto* mV = Sat(Mul(Sub(Const(0.5f, 180), Abs(Sub(v, Const(0.5f, 180), 180), 180), 180), Const(60.f, 180), 180), 180);
	auto* dentro = Mul(mU, mV, 150);

	// Color de relleno fuera de la ortofoto (pradera/roca neutra).
	auto* relleno = Cast<UMaterialExpressionConstant3Vector>(New(UMaterialExpressionConstant3Vector::StaticClass(), 260));
	relleno->Constant = FLinearColor(0.18f, 0.22f, 0.13f);

	auto* base = Cast<UMaterialExpressionLinearInterpolate>(New(UMaterialExpressionLinearInterpolate::StaticClass(), 60));
	ML::ConnectMaterialExpressions(relleno, TEXT(""), base, TEXT("A"));
	ML::ConnectMaterialExpressions(tex, TEXT("RGB"), base, TEXT("B"));
	ML::ConnectMaterialExpressions(dentro, TEXT(""), base, TEXT("Alpha"));

	// --- Detalle de cerca: textura tileada que modula el brillo, fundida por distancia ---
	// La ortofoto (25 cm/px) se ve borrosa a ras de suelo; este detalle le da nitidez sin
	// cambiar el color medio. Lejos se desvanece para no ver el patrón repetido.
	auto* dUV = Bin(UMaterialExpressionAppendVector::StaticClass(),
		Mul(wX, Const(1.f / 200.f, 360), 360), Mul(wY, Const(1.f / 200.f, 420), 420), 390);   // tile 2 m
	auto* det = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 480));
	det->ParameterName = TEXT("Detalle");
	det->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	ML::ConnectMaterialExpressions(dUV, TEXT(""), det, TEXT("UVs"));
	auto* dLum = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), 540)); dLum->R=true; dLum->G=false; dLum->B=false; dLum->A=false;
	ML::ConnectMaterialExpressions(det, TEXT("RGB"), dLum, TEXT(""));
	auto* centrado = Sub(dLum, Const(0.5f, 540), 540);   // [-0.5..0.5]

	// Fundido por distancia: cerca 1, lejos 0 (a partir de ~60 m).
	auto* depth = New(UMaterialExpressionPixelDepth::StaticClass(), 600);
	auto* fade  = Sat(Mul(Sub(Const(6000.f, 600), depth, 600), Const(1.f / 4000.f, 600), 600), 600);

	auto* factor = Add(Const(1.f, 460), Mul(centrado, fade, 500), 480);   // 1 ± detalle*fade
	ML::ConnectMaterialProperty(Mul(base, factor, 60), TEXT(""), MP_BaseColor);

	// Relieve de cerca: normal map tileado, fundido a plano (0,0,1) por distancia.
	auto* plano = Cast<UMaterialExpressionConstant3Vector>(New(UMaterialExpressionConstant3Vector::StaticClass(), 660)); plano->Constant = FLinearColor(0, 0, 1);
	ML::ConnectMaterialProperty(plano, TEXT(""), MP_Normal);

	auto* rough = Const(0.85f, 300);
	ML::ConnectMaterialProperty(rough, TEXT(""), MP_Roughness);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[TerrenoOrto] material creado en %s (asignalo como Landscape Material)"), *Ruta);
	return true;
}
