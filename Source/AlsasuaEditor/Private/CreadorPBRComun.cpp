// CreadorPBRComun.cpp (sólo editor)
#include "CreadorPBRComun.h"
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

using ML = UMaterialEditingLibrary;

namespace
{
	/** Qué tipo de dato lleva cada mapa: cambia compresión, sRGB y sampler. */
	enum class EMapa { Color, Normal, Lineal };

	/**
	 * Carga una textura de Content/Textures y corrige sus ajustes de import.
	 *
	 * La ruta de un asset es /Carpeta/Paquete.Objeto, y en las PNG
	 * auto-importadas el objeto repite el nombre del paquete:
	 * T_StoneWall_Color.T_StoneWall_Color. Escribirlo como
	 * T_StoneWall_Color.StoneWall_Color no da error — LoadObject devuelve null,
	 * el material cae a DefaultTexture y el muro sale gris.
	 */
	UTexture2D* CargarTextura(const FString& Set, const TCHAR* Sufijo, EMapa Tipo)
	{
		const FString Nombre = FString::Printf(TEXT("T_%s_%s"), *Set, Sufijo);
		const FString Ruta = FString::Printf(TEXT("/Game/Textures/%s.%s"), *Nombre, *Nombre);

		UTexture2D* Tex = LoadObject<UTexture2D>(nullptr, *Ruta);
		if (!Tex) return nullptr;

		// El auto-import de PNG deja todo como sRGB/TC_Default. Un mapa de
		// roughness o AO leído como sRGB da brillos mal, y un normal sin
		// TC_Normalmap se comprime como color y arruina la iluminación.
		const TextureCompressionSettings Compresion =
			(Tipo == EMapa::Normal) ? TC_Normalmap :
			(Tipo == EMapa::Lineal) ? TC_Masks : TC_Default;
		const uint8 SRGBDeseado = (Tipo == EMapa::Color) ? 1 : 0;

		if (Tex->SRGB != SRGBDeseado || Tex->CompressionSettings != Compresion)
		{
			Tex->SRGB = SRGBDeseado;
			Tex->CompressionSettings = Compresion;
			Tex->PostEditChange();
			Tex->MarkPackageDirty();
		}
		return Tex;
	}

	EMaterialSamplerType SamplerDe(EMapa Tipo)
	{
		switch (Tipo)
		{
		case EMapa::Normal: return SAMPLERTYPE_Normal;
		case EMapa::Lineal: return SAMPLERTYPE_Masks;
		default:            return SAMPLERTYPE_Color;
		}
	}

	/** Muestreador con la textura del set ya cargada y el UV conectado. */
	UMaterialExpression* Muestrear(UMaterial* Mat, const FString& Set, const TCHAR* Sufijo,
		EMapa Tipo, const TCHAR* Parametro, UMaterialExpression* UV, int32 Y)
	{
		UTexture2D* Tex = CargarTextura(Set, Sufijo, Tipo);
		if (!Tex) return nullptr;

		auto* S = Cast<UMaterialExpressionTextureSampleParameter2D>(
			ML::CreateMaterialExpression(Mat, UMaterialExpressionTextureSampleParameter2D::StaticClass(), -500, Y));
		if (!S) return nullptr;

		S->ParameterName = Parametro;
		S->SamplerType = SamplerDe(Tipo);
		S->Texture = Tex;
		ML::ConnectMaterialExpressions(UV, TEXT(""), S, TEXT("UVs"));
		return S;
	}
}

UMaterialExpression* AlsasuaPBR::UVMundo(UMaterial* Mat, float TileCm, int32 X, int32 Y)
{
	if (!Mat) return nullptr;

	auto* WP = ML::CreateMaterialExpression(Mat, UMaterialExpressionWorldPosition::StaticClass(), X, Y);

	auto* mX = Cast<UMaterialExpressionComponentMask>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionComponentMask::StaticClass(), X + 100, Y - 40));
	mX->R = true; mX->G = false; mX->B = false; mX->A = false;

	auto* mY = Cast<UMaterialExpressionComponentMask>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionComponentMask::StaticClass(), X + 100, Y + 40));
	mY->R = false; mY->G = true; mY->B = false; mY->A = false;

	ML::ConnectMaterialExpressions(WP, TEXT(""), mX, TEXT(""));
	ML::ConnectMaterialExpressions(WP, TEXT(""), mY, TEXT(""));

	auto* Escala = Cast<UMaterialExpressionConstant>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), X + 100, Y));
	Escala->R = 1.f / FMath::Max(TileCm, 1.f);

	auto* u = ML::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), X + 200, Y - 40);
	ML::ConnectMaterialExpressions(mX, TEXT(""), u, TEXT("A"));
	ML::ConnectMaterialExpressions(Escala, TEXT(""), u, TEXT("B"));

	auto* v = ML::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), X + 200, Y + 40);
	ML::ConnectMaterialExpressions(mY, TEXT(""), v, TEXT("A"));
	ML::ConnectMaterialExpressions(Escala, TEXT(""), v, TEXT("B"));

	auto* uv = ML::CreateMaterialExpression(Mat, UMaterialExpressionAppendVector::StaticClass(), X + 300, Y);
	ML::ConnectMaterialExpressions(u, TEXT(""), uv, TEXT("A"));
	ML::ConnectMaterialExpressions(v, TEXT(""), uv, TEXT("B"));
	return uv;
}

UMaterialExpression* AlsasuaPBR::Clima(UMaterial* Mat, const TCHAR* Parametro, int32 X, int32 Y)
{
	UMaterialParameterCollection* MPC =
		LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
	if (!Mat || !MPC) return nullptr;

	auto* P = Cast<UMaterialExpressionCollectionParameter>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionCollectionParameter::StaticClass(), X, Y));
	if (!P) return nullptr;

	P->Collection = MPC;
	P->ParameterName = Parametro;
	for (const FCollectionScalarParameter& S : MPC->ScalarParameters)
	{
		if (S.ParameterName == Parametro) { P->ParameterId = S.Id; break; }
	}
	return P;
}

bool AlsasuaPBR::Cablear(UMaterial* Mat, const FOpciones& Op)
{
	if (!Mat) return false;

	UMaterialExpression* UV = UVMundo(Mat, Op.TileCm, -1100, 0);

	UMaterialExpression* Color = Muestrear(Mat, Op.Set, TEXT("Color"), EMapa::Color, TEXT("BaseTexture"), UV, -200);
	if (!Color)
	{
		UE_LOG(LogTemp, Warning, TEXT("[PBR] falta /Game/Textures/T_%s_Color: abre el editor para que auto-importe Content/Textures."), *Op.Set);
		return false;
	}

	// Color base: opcionalmente tintado (las fachadas llevan su color por vértice).
	UMaterialExpression* ColorFinal = Color;
	if (Op.Tinte)
	{
		auto* M = ML::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), -300, -200);
		ML::ConnectMaterialExpressions(Color, TEXT("RGB"), M, TEXT("A"));
		ML::ConnectMaterialExpressions(Op.Tinte, TEXT(""), M, TEXT("B"));
		ColorFinal = M;
		ML::ConnectMaterialProperty(ColorFinal, TEXT(""), MP_BaseColor);
	}
	else
	{
		ML::ConnectMaterialProperty(Color, TEXT("RGB"), MP_BaseColor);
	}

	if (UMaterialExpression* Normal = Muestrear(Mat, Op.Set, TEXT("Normal"), EMapa::Normal, TEXT("NormalTex"), UV, 0))
	{
		ML::ConnectMaterialProperty(Normal, TEXT("RGB"), MP_Normal);
	}

	// Roughness del mapa, escalada y llevada a RoughnessMojado con la lluvia.
	// Antes era una constante por material: bajo Lumen todo parecía plástico.
	UMaterialExpression* Rough = Muestrear(Mat, Op.Set, TEXT("Roughness"), EMapa::Lineal, TEXT("RoughnessTex"), UV, 200);
	UMaterialExpression* RoughSeco = nullptr;
	if (Rough)
	{
		if (FMath::IsNearlyEqual(Op.EscalaRoughness, 1.f))
		{
			RoughSeco = Rough;
		}
		else
		{
			auto* Esc = Cast<UMaterialExpressionConstant>(
				ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -300, 240));
			Esc->R = Op.EscalaRoughness;
			auto* M = ML::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), -200, 200);
			ML::ConnectMaterialExpressions(Rough, TEXT("R"), M, TEXT("A"));
			ML::ConnectMaterialExpressions(Esc, TEXT(""), M, TEXT("B"));
			RoughSeco = M;
		}
	}

	if (UMaterialExpression* Wet = Clima(Mat, TEXT("Wetness"), -300, 320))
	{
		auto* Mojado = Cast<UMaterialExpressionConstant>(
			ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -300, 360));
		Mojado->R = Op.RoughnessMojado;

		auto* Lerp = ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), -100, 300);
		if (RoughSeco)
		{
			ML::ConnectMaterialExpressions(RoughSeco, (RoughSeco == Rough) ? TEXT("R") : TEXT(""), Lerp, TEXT("A"));
		}
		else
		{
			auto* Seco = Cast<UMaterialExpressionConstant>(
				ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -300, 280));
			Seco->R = FMath::Clamp(Op.EscalaRoughness, 0.f, 1.f);
			ML::ConnectMaterialExpressions(Seco, TEXT(""), Lerp, TEXT("A"));
		}
		ML::ConnectMaterialExpressions(Mojado, TEXT(""), Lerp, TEXT("B"));
		ML::ConnectMaterialExpressions(Wet, TEXT(""), Lerp, TEXT("Alpha"));
		ML::ConnectMaterialProperty(Lerp, TEXT(""), MP_Roughness);
	}
	else if (RoughSeco)
	{
		ML::ConnectMaterialProperty(RoughSeco, (RoughSeco == Rough) ? TEXT("R") : TEXT(""), MP_Roughness);
	}

	// AO: no todos los sets lo traen (Concrete, Wood y MetalPlate no).
	if (Op.bUsarAO)
	{
		if (UMaterialExpression* AO = Muestrear(Mat, Op.Set, TEXT("AO"), EMapa::Lineal, TEXT("AOTex"), UV, 400))
		{
			ML::ConnectMaterialProperty(AO, TEXT("R"), MP_AmbientOcclusion);
		}
	}

	return true;
}
