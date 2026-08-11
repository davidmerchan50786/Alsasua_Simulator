// CreadorMaterialRelieveLejano.cpp (sólo editor)
#include "CreadorMaterialRelieveLejano.h"
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
#include "Materials/MaterialExpressionMax.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionAppendVector.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionVertexNormalWS.h"
#include "Engine/Texture2D.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

namespace
{
// Caja del relieve lejano en mundo (cm), la misma que escribe
// Tools/DescargarRelieveLejano.py en su meta: centro (191800, 857000), semilado 30 km.
// La textura va con el SUR arriba, igual que las otras ortofotos: v=0 en Ymin.
constexpr float LEJ_XMIN_CM = 191800.f - 3000000.f;
constexpr float LEJ_YMIN_CM = 857000.f - 3000000.f;
constexpr float LEJ_RANGO_CM = 6000000.f;

// Alturas de mundo (cm) donde cambia el color procedural. worldZ = alt_m*100 - 51133,
// así que 600 m -> 8867 cm, 1100 m -> 58867 cm.
constexpr float Z_PRADO_CM = 8867.f;
constexpr float Z_CUMBRE_CM = 58867.f;

// Distancias (cm desde el centro del mundo) del fundido foto -> procedural.
constexpr float FOTO_HASTA_CM = 800000.f;    // 8 km: foto pura
constexpr float PROC_DESDE_CM = 1800000.f;   // 18 km: procedural puro
}

bool UCreadorMaterialRelieveLejano::CrearMaterialRelieveLejano()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Ruta    = Carpeta / TEXT("M_Relieve_Lejano");

	if (UEditorAssetLibrary::DoesAssetExist(Ruta)) UEditorAssetLibrary::DeleteAsset(Ruta);
	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(TEXT("M_Relieve_Lejano"), Carpeta,
		UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[RelieveLejano] no pude crear el material")); return false; }

	int32 gx = -1000, gy = -200;
	auto New   = [&](UClass* C, int32 y){ gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y){ auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };
	auto Bin   = [&](UClass* C, UMaterialExpression* A, UMaterialExpression* B, int32 y){ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(A, TEXT(""), e, TEXT("A")); ML::ConnectMaterialExpressions(B, TEXT(""), e, TEXT("B")); return e; };
	auto Un    = [&](UClass* C, UMaterialExpression* X, int32 y){ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(X, TEXT(""), e, TEXT("")); return e; };
	auto Mul   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionMultiply::StaticClass(), A, B, y); };
	auto Sub   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionSubtract::StaticClass(), A, B, y); };
	auto Add   = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionAdd::StaticClass(), A, B, y); };
	auto MaxE  = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionMax::StaticClass(), A, B, y); };
	auto Abs   = [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionAbs::StaticClass(), X, y); };
	auto Sat   = [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionSaturate::StaticClass(), X, y); };
	auto Lerp  = [&](UMaterialExpression* A, UMaterialExpression* B, UMaterialExpression* T, int32 y)
	{
		auto* l = Cast<UMaterialExpressionLinearInterpolate>(New(UMaterialExpressionLinearInterpolate::StaticClass(), y));
		ML::ConnectMaterialExpressions(A, TEXT(""), l, TEXT("A"));
		ML::ConnectMaterialExpressions(B, TEXT(""), l, TEXT("B"));
		ML::ConnectMaterialExpressions(T, TEXT(""), l, TEXT("Alpha"));
		return (UMaterialExpression*)l;
	};
	auto Color = [&](float r, float g, float b, int32 y)
	{
		auto* c = Cast<UMaterialExpressionConstant3Vector>(New(UMaterialExpressionConstant3Vector::StaticClass(), y));
		c->Constant = FLinearColor(r, g, b);
		return (UMaterialExpression*)c;
	};

	// --- Posición de mundo descompuesta ---
	auto* WP = New(UMaterialExpressionWorldPosition::StaticClass(), -200);
	auto* wX = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -240)); wX->R=true;  wX->G=false; wX->B=false; wX->A=false;
	auto* wY = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -160)); wY->R=false; wY->G=true;  wY->B=false; wY->A=false;
	auto* wZ = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -120)); wZ->R=false; wZ->G=false; wZ->B=true;  wZ->A=false;
	ML::ConnectMaterialExpressions(WP, TEXT(""), wX, TEXT(""));
	ML::ConnectMaterialExpressions(WP, TEXT(""), wY, TEXT(""));
	ML::ConnectMaterialExpressions(WP, TEXT(""), wZ, TEXT(""));

	// --- Ortofoto PNOA de los 60 km, proyectada en planta ---
	auto* u = Mul(Sub(wX, Const(LEJ_XMIN_CM, -160), -160), Const(1.f / LEJ_RANGO_CM, -160), -150);
	auto* v = Mul(Sub(wY, Const(LEJ_YMIN_CM, -240), -240), Const(1.f / LEJ_RANGO_CM, -240), -250);
	auto* uv = Bin(UMaterialExpressionAppendVector::StaticClass(), u, v, -200);

	auto* tex = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 0));
	tex->ParameterName = TEXT("OrtofotoLejana");
	if (UTexture2D* T = LoadObject<UTexture2D>(nullptr, TEXT("/Game/Terreno/T_Relieve_Lejano.T_Relieve_Lejano"))) tex->Texture = T;
	else tex->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	ML::ConnectMaterialExpressions(uv, TEXT(""), tex, TEXT("UVs"));

	// --- Color procedural por altitud y pendiente ---
	// Altitud: prado -> roca -> caliza de cumbre. Las sierras de aquí son caliza
	// clara, no nieve, así que la cumbre tira a gris hueso y no a blanco.
	auto* tAlt = Sat(Mul(Sub(wZ, Const(Z_PRADO_CM, 300), 300),
	                     Const(1.f / (Z_CUMBRE_CM - Z_PRADO_CM), 300), 300), 300);
	auto* porAltura = Lerp(Color(0.09f, 0.14f, 0.07f, 340),    // prado y hayedo
	                       Color(0.42f, 0.40f, 0.36f, 380),    // caliza
	                       tAlt, 360);

	// Pendiente: donde la cara es vertical asoma roca desnuda pase lo que pase.
	auto* VN = New(UMaterialExpressionVertexNormalWS::StaticClass(), 440);
	auto* nZ = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), 460)); nZ->R=false; nZ->G=false; nZ->B=true; nZ->A=false;
	ML::ConnectMaterialExpressions(VN, TEXT(""), nZ, TEXT(""));
	auto* tRoca = Sat(Mul(Sub(Const(0.82f, 500), Abs(nZ, 500), 500), Const(4.f, 500), 500), 500);
	auto* proc = Lerp(porAltura, Color(0.30f, 0.28f, 0.25f, 540), tRoca, 560);

	// --- Fundido foto -> procedural por distancia al centro del mundo ---
	// Chebyshev, la misma forma de cuadrado que tiene el anillo, para que la
	// transición sea concéntrica con la costura y no una circunferencia que la corta.
	auto* dCheb = MaxE(Abs(Sub(wX, Const(191800.f, 620), 620), 620),
	                   Abs(Sub(wY, Const(857000.f, 660), 660), 660), 640);
	auto* tFoto = Sat(Mul(Sub(dCheb, Const(FOTO_HASTA_CM, 700), 700),
	                      Const(1.f / (PROC_DESDE_CM - FOTO_HASTA_CM), 700), 700), 700);

	auto* base = Lerp(tex, proc, tFoto, 60);
	ML::ConnectMaterialProperty(base, TEXT(""), MP_BaseColor);

	// Mate y sin especular: es terreno a kilómetros, cualquier brillo canta como
	// un plástico y encima delata que es una superficie plana con foto encima.
	ML::ConnectMaterialProperty(Const(0.95f, 760), TEXT(""), MP_Roughness);
	ML::ConnectMaterialProperty(Const(0.f, 800), TEXT(""), MP_Specular);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[RelieveLejano] material creado en %s"), *Ruta);
	return true;
}
