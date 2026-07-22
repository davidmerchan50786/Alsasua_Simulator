// CreadorMaterialFachada.cpp (sólo editor)
#include "CreadorMaterialFachada.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionAbs.h"
#include "Materials/MaterialExpressionFrac.h"
#include "Materials/MaterialExpressionSaturate.h"
#include "Materials/MaterialExpressionFloor.h"
#include "Materials/MaterialExpressionSine.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialExpressionPixelNormalWS.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

bool UCreadorMaterialFachada::CrearMaterialFachada()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Ruta    = Carpeta / TEXT("M_Fachada");

	UMaterialParameterCollection* MPC =
		LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
	if (!MPC)
		UE_LOG(LogTemp, Warning, TEXT("[Fachada] no hay MPC_Clima: ejecuta CrearMaterialEdificio() primero. Sin ventanas/mojado."));

	if (UEditorAssetLibrary::DoesAssetExist(Ruta)) UEditorAssetLibrary::DeleteAsset(Ruta);
	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(TEXT("M_Fachada"), Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Fachada] no pude crear el material")); return false; }

	int32 gx = -900, gy = -400;
	auto New = [&](UClass* C, int32 y) { gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y) { auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };
	auto Bin = [&](UClass* C, UMaterialExpression* A, UMaterialExpression* B, int32 y)
	{ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(A, TEXT(""), e, TEXT("A")); ML::ConnectMaterialExpressions(B, TEXT(""), e, TEXT("B")); return e; };
	auto Un = [&](UClass* C, UMaterialExpression* X, int32 y)
	{ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(X, TEXT(""), e, TEXT("")); return e; };

	auto Mul = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionMultiply::StaticClass(), A, B, y); };
	auto Sub = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionSubtract::StaticClass(), A, B, y); };
	auto Add = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionAdd::StaticClass(), A, B, y); };
	auto Frac= [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionFrac::StaticClass(), X, y); };
	auto Abs = [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionAbs::StaticClass(), X, y); };
	auto Sat = [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionSaturate::StaticClass(), X, y); };
	auto Floor=[&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionFloor::StaticClass(), X, y); };
	auto Sine= [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionSine::StaticClass(), X, y); };

	// UV0 -> U, V
	auto* UV = New(UMaterialExpressionTextureCoordinate::StaticClass(), -400);
	auto* mU = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -440)); mU->R = true;  mU->G = false; mU->B = false; mU->A = false;
	auto* mV = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -360)); mV->R = false; mV->G = true;  mV->B = false; mV->A = false;
	ML::ConnectMaterialExpressions(UV, TEXT(""), mU, TEXT(""));
	ML::ConnectMaterialExpressions(UV, TEXT(""), mV, TEXT(""));

	// Banda de ventana en un eje: ~1 dentro del cristal, 0 en el montante.
	auto Banda = [&](UMaterialExpression* coord, float invPeriodo, float halfband, int32 y) -> UMaterialExpression*
	{
		auto* s   = Mul(coord, Const(invPeriodo, y), y);
		auto* f   = Frac(s, y);
		auto* c   = Sub(f, Const(0.5f, y), y);
		auto* a   = Abs(c, y);
		auto* inv = Sub(Const(halfband, y), a, y);
		return Sat(Mul(inv, Const(20.f, y), y), y);   // sharpen + saturate
	};
	auto* bandaU = Banda(mU, 1.f / 2.6f, 0.32f, -440);  // ventana cada 2.6 m
	auto* bandaV = Banda(mV, 1.f / 3.1f, 0.30f, -360);  // una planta cada 3.1 m
	auto* ventana = Mul(bandaU, bandaV, -400);

	// Encendido pseudo-aleatorio por ventana: hash de la celda (floorU, floorV).
	// hash = frac(sin(cellU*12.9898 + cellV*78.233) * 43758.5453); encendida si > umbral.
	auto* cellU = Floor(Mul(mU, Const(1.f / 2.6f, -300), -300), -300);
	auto* cellV = Floor(Mul(mV, Const(1.f / 3.1f, -260), -260), -260);
	auto* hsum  = Add(Mul(cellU, Const(12.9898f, -300), -300), Mul(cellV, Const(78.233f, -260), -260), -280);
	auto* hash  = Frac(Mul(Sine(hsum, -280), Const(43758.545f, -280), -280), -280);
	auto* encendida = Sat(Mul(Sub(hash, Const(0.45f, -240), -240), Const(50.f, -240), -240), -240);  // ~55% encendidas
	auto* ventanaOn = Mul(ventana, encendida, -400);

	// Solo en caras verticales: |Normal.Z| pequeño -> muro.
	auto* nrm = New(UMaterialExpressionPixelNormalWS::StaticClass(), 40);
	auto* nZ  = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), 40)); nZ->R=false; nZ->G=false; nZ->B=true; nZ->A=false;
	ML::ConnectMaterialExpressions(nrm, TEXT(""), nZ, TEXT(""));
	auto* muro = Sat(Mul(Sub(Const(0.45f, 80), Abs(nZ, 80), 80), Const(10.f, 80), 80), 80);

	// Color de ventana cálido e intensidad.
	auto* winCol = Cast<UMaterialExpressionConstant3Vector>(New(UMaterialExpressionConstant3Vector::StaticClass(), 200));
	winCol->Constant = FLinearColor(1.0f, 0.82f, 0.5f);

	// Emisivo = ventanaEncendida * muro * Night * color * 3
	UMaterialExpression* gate = Mul(ventanaOn, muro, 120);
	if (MPC)
	{
		auto* night = Cast<UMaterialExpressionCollectionParameter>(New(UMaterialExpressionCollectionParameter::StaticClass(), 160));
		night->Collection = MPC; night->ParameterName = TEXT("Night");
		gate = Mul(gate, night, 140);
	}
	auto* emis = Mul(Mul(winCol, gate, 200), Const(3.f, 220), 210);
	ML::ConnectMaterialProperty(emis, TEXT(""), MP_EmissiveColor);

	// --- Base + mojado (como el suelo) ---
	auto* VC = New(UMaterialExpressionVertexColor::StaticClass(), -200);
	UMaterialExpression* baseFactor = Const(1.f, -160);
	UMaterialExpression* rough = Const(0.78f, 320);
	if (MPC)
	{
		auto* wet = Cast<UMaterialExpressionCollectionParameter>(New(UMaterialExpressionCollectionParameter::StaticClass(), -120));
		wet->Collection = MPC; wet->ParameterName = TEXT("Wetness");
		auto* lc = Cast<UMaterialExpressionLinearInterpolate>(New(UMaterialExpressionLinearInterpolate::StaticClass(), -160));
		ML::ConnectMaterialExpressions(Const(1.0f, -160), TEXT(""), lc, TEXT("A"));
		ML::ConnectMaterialExpressions(Const(0.55f, -140), TEXT(""), lc, TEXT("B"));
		ML::ConnectMaterialExpressions(wet, TEXT(""), lc, TEXT("Alpha"));
		baseFactor = lc;
		auto* lr = Cast<UMaterialExpressionLinearInterpolate>(New(UMaterialExpressionLinearInterpolate::StaticClass(), 320));
		ML::ConnectMaterialExpressions(Const(0.78f, 320), TEXT(""), lr, TEXT("A"));
		ML::ConnectMaterialExpressions(Const(0.15f, 340), TEXT(""), lr, TEXT("B"));
		ML::ConnectMaterialExpressions(wet, TEXT(""), lr, TEXT("Alpha"));
		rough = lr;
	}
	ML::ConnectMaterialProperty(Mul(VC, baseFactor, -180), TEXT(""), MP_BaseColor);
	ML::ConnectMaterialProperty(rough, TEXT(""), MP_Roughness);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[Fachada] material creado en %s"), *Ruta);
	return true;
}
