// CreadorMaterialAgua.cpp (sólo editor)
// Material de agua mejorado: flujo animado, espuma en orillas, profundidad variable.
#include "CreadorMaterialAgua.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "Materials/MaterialExpressionTextureCoordinate.h"
#include "Materials/MaterialExpressionPanner.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionAdd.h"
#include "Materials/MaterialExpressionSubtract.h"
#include "Materials/MaterialExpressionComponentMask.h"
#include "Materials/MaterialExpressionTextureSampleParameter2D.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionClamp.h"
#include "Materials/MaterialExpressionWorldPosition.h"
#include "Materials/MaterialExpressionPixelDepth.h"
#include "Engine/Texture2D.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"

static void FijarEnumByte(UObject* O, const TCHAR* Nombre, uint8 Valor)
{
	if (FByteProperty* P = CastField<FByteProperty>(O->GetClass()->FindPropertyByName(FName(Nombre))))
		P->SetPropertyValue_InContainer(O, Valor);
	else if (FEnumProperty* E = CastField<FEnumProperty>(O->GetClass()->FindPropertyByName(FName(Nombre))))
		E->GetUnderlyingProperty()->SetIntPropertyValue(E->ContainerPtrToValuePtr<void>(O), (int64)Valor);
}

static void FijarBool(UObject* O, const TCHAR* Nombre, bool Valor)
{
	if (FBoolProperty* P = CastField<FBoolProperty>(O->GetClass()->FindPropertyByName(FName(Nombre))))
		P->SetPropertyValue_InContainer(O, Valor);
}

bool UCreadorMaterialAgua::CrearMaterialAgua()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre  = TEXT("M_AguaRio");
	const FString Ruta    = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterialFactoryNew* Fab = NewObject<UMaterialFactoryNew>();
	UObject* Obj = AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), Fab);
	UMaterial* Mat = Cast<UMaterial>(Obj);
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Agua] no pude crear el material")); return false; }

	// Translúcido, iluminado, doble cara
	FijarEnumByte(Mat, TEXT("BlendMode"), (uint8)BLEND_Translucent);
	Mat->SetShadingModel(MSM_DefaultLit);
	FijarBool(Mat, TEXT("TwoSided"), true);

	using ML = UMaterialEditingLibrary;
	int32 gx = -900, gy = -200;
	auto New   = [&](UClass* C, int32 y){ gy = y; return ML::CreateMaterialExpression(Mat, C, gx, y); };
	auto Const = [&](float v, int32 y){ auto* c = Cast<UMaterialExpressionConstant>(New(UMaterialExpressionConstant::StaticClass(), y)); c->R = v; return (UMaterialExpression*)c; };
	auto Const3= [&](FLinearColor c, int32 y){ auto* e = Cast<UMaterialExpressionConstant3Vector>(New(UMaterialExpressionConstant3Vector::StaticClass(), y)); e->Constant = c; return (UMaterialExpression*)e; };
	auto Bin   = [&](UClass* C, UMaterialExpression* A, UMaterialExpression* B, int32 y)
	{ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(A, TEXT(""), e, TEXT("A")); ML::ConnectMaterialExpressions(B, TEXT(""), e, TEXT("B")); return e; };
	auto Un    = [&](UClass* C, UMaterialExpression* X, int32 y)
	{ UMaterialExpression* e = New(C, y); ML::ConnectMaterialExpressions(X, TEXT(""), e, TEXT("")); return e; };
	auto Mul = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionMultiply::StaticClass(), A, B, y); };
	auto Sub = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionSubtract::StaticClass(), A, B, y); };
	auto Add = [&](UMaterialExpression* A, UMaterialExpression* B, int32 y){ return Bin(UMaterialExpressionAdd::StaticClass(), A, B, y); };
	auto Sat = [&](UMaterialExpression* X, int32 y){ return Un(UMaterialExpressionSaturate::StaticClass(), X, y); };

	// ── UV coords from world position (X, Y → planar projection) ──
	auto* WP = New(UMaterialExpressionWorldPosition::StaticClass(), -200);
	auto* wX = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -240));
	wX->R = true; wX->G = false; wX->B = false; wX->A = false;
	auto* wY = Cast<UMaterialExpressionComponentMask>(New(UMaterialExpressionComponentMask::StaticClass(), -160));
	wY->R = false; wY->G = true; wY->B = false; wY->A = false;
	ML::ConnectMaterialExpressions(WP, TEXT(""), wX, TEXT(""));
	ML::ConnectMaterialExpressions(WP, TEXT(""), wY, TEXT(""));

	auto* uvBase = Bin(UMaterialExpressionAppendVector::StaticClass(),
		Mul(wX, Const(1.f / 400.f, -240), -240),
		Mul(wY, Const(1.f / 400.f, -160)), -200);  // 4m tiling

	// ── Animated normal 1: slow flow in X direction ──
	auto* panner1 = Cast<UMaterialExpressionPanner>(New(UMaterialExpressionPanner::StaticClass(), -100));
	panner1->SpeedX = 0.08f; panner1->SpeedY = 0.02f;
	ML::ConnectMaterialExpressions(uvBase, TEXT(""), panner1, TEXT("Coordinate"));

	auto* norm1 = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 0));
	norm1->ParameterName = TEXT("NormalFlow1");
	norm1->SamplerType = SAMPLERTYPE_Normal;
	norm1->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	ML::ConnectMaterialExpressions(panner1, TEXT(""), norm1, TEXT("UVs"));

	// ── Animated normal 2: faster flow in slightly different direction ──
	auto* panner2 = Cast<UMaterialExpressionPanner>(New(UMaterialExpressionPanner::StaticClass(), -100, 100));
	panner2->SpeedX = 0.12f; panner2->SpeedY = -0.03f;
	ML::ConnectMaterialExpressions(uvBase, TEXT(""), panner2, TEXT("Coordinate"));

	auto* norm2 = Cast<UMaterialExpressionTextureSampleParameter2D>(New(UMaterialExpressionTextureSampleParameter2D::StaticClass(), 100, 100));
	norm2->ParameterName = TEXT("NormalFlow2");
	norm2->SamplerType = SAMPLERTYPE_Normal;
	norm2->Texture = LoadObject<UTexture2D>(nullptr, TEXT("/Engine/EngineResources/DefaultTexture.DefaultTexture"));
	ML::ConnectMaterialExpressions(panner2, TEXT(""), norm2, TEXT("UVs"));

	// ── Blend normals: average of two animated normals ──
	auto* blendedNorm = Cast<UMaterialExpressionLinearInterpolate>(
		New(UMaterialExpressionLinearInterpolate::StaticClass(), 200, 0));
	ML::ConnectMaterialExpressions(norm1, TEXT("RGB"), blendedNorm, TEXT("A"));
	ML::ConnectMaterialExpressions(norm2, TEXT("RGB"), blendedNorm, TEXT("B"));
	Const(0.5f, 40);
	ML::ConnectMaterialExpressions(Const(0.5f, 40), TEXT(""), blendedNorm, TEXT("Alpha"));
	ML::ConnectMaterialProperty(blendedNorm, TEXT(""), MP_Normal);

	// ── Water color: depth-based (deeper = darker) ──
	// Shallow: greenish. Deep: dark blue.
	auto* shallowCol = Const3(FLinearColor(0.04f, 0.18f, 0.22f), -100);
	auto* deepCol    = Const3(FLinearColor(0.008f, 0.04f, 0.12f), -60);

	// Use pixel depth for depth-based color
	auto* depth = New(UMaterialExpressionPixelDepth::StaticClass(), 0);
	auto* depthFactor = Sat(Mul(depth, Const(1.f / 5000.f, 40), 20), 20);

	auto* waterCol = Cast<UMaterialExpressionLinearInterpolate>(
		New(UMaterialExpressionLinearInterpolate::StaticClass(), 60));
	ML::ConnectMaterialExpressions(shallowCol, TEXT(""), waterCol, TEXT("A"));
	ML::ConnectMaterialExpressions(deepCol, TEXT(""), waterCol, TEXT("B"));
	ML::ConnectMaterialExpressions(depthFactor, TEXT(""), waterCol, TEXT("Alpha"));
	ML::ConnectMaterialProperty(waterCol, TEXT(""), MP_BaseColor);

	// ── Roughness: very smooth but with subtle variation ──
	auto* roughBase = Const(0.02f, 160);
	auto* roughVar  = Mul(Const(0.01f, 200),
		Add(Sat(Mul(norm1, Const(0.5f, 220), 220), 220), Const(-0.5f, 240)), 230);
	auto* rough = Add(roughBase, roughVar, 200);
	ML::ConnectMaterialProperty(rough, TEXT(""), MP_Roughness);

	// ── Specular ──
	ML::ConnectMaterialProperty(Const(1.f, 260), TEXT(""), MP_Specular);

	// ── Opacity: Fresnel + depth fade at edges ──
	// Fresnel: transparent when looking straight down, reflective at grazing angles
	auto* fresnel = Cast<UMaterialExpressionFresnel>(New(UMaterialExpressionFresnel::StaticClass(), 300));
	fresnel->Exponent = 3.f;
	fresnel->BaseReflectFraction = 0.4f;

	// Depth fade: make water transparent near riverbed
	auto* depthFade = Sat(Sub(Const(1.f, 380), Mul(depth, Const(1.f / 2000.f, 400), 400)), 400);

	auto* opacity = Mul(fresnel, depthFade, 350);
	ML::ConnectMaterialProperty(opacity, TEXT(""), MP_Opacity);

	// ── Emissive: subtle foam/whitecap near edges ──
	// Use fresnel to create foam at glancing angles
	auto* foam = Sat(Sub(fresnel, Const(0.7f, 460), 460), 460);
	auto* foamColor = Mul(foam, Const3(FLinearColor(0.5f, 0.55f, 0.5f), 500), 480);
	ML::ConnectMaterialProperty(foamColor, TEXT(""), MP_EmissiveColor);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);

	UE_LOG(LogTemp, Log, TEXT("[Agua] material mejorado creado en %s (flujo animado + espuma)"), *Ruta);
	return true;
}
