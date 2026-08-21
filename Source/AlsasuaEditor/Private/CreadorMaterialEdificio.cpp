// CreadorMaterialEdificio.cpp (sólo editor)
// Crea el MPC_Clima (escalar "Wetness") y el material vertex-color M_Edificio que
// lo lee: con lluvia, el suelo se oscurece y baja su rugosidad (aspecto mojado).
#include "CreadorMaterialEdificio.h"
#include "CreadorPBRComun.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionVertexColor.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "Materials/MaterialExpressionMultiply.h"
#include "Materials/MaterialExpressionCollectionParameter.h"
#include "Materials/MaterialParameterCollection.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

// Crea (o reutiliza) /Game/Materiales/MPC_Clima con el escalar "Wetness" y "Night".
static UMaterialParameterCollection* AsegurarMPCClima(IAssetTools& AT)
{
	const FString Ruta = TEXT("/Game/Materiales/MPC_Clima");
	UMaterialParameterCollection* MPC =
		LoadObject<UMaterialParameterCollection>(nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
	if (!MPC)
	{
		UObject* O = AT.CreateAsset(TEXT("MPC_Clima"), TEXT("/Game/Materiales"),
			UMaterialParameterCollection::StaticClass(), nullptr);
		MPC = Cast<UMaterialParameterCollection>(O);
	}
	if (MPC)
	{
		auto TieneEscalar = [&](FName Nombre)
		{
			for (const FCollectionScalarParameter& P : MPC->ScalarParameters)
				if (P.ParameterName == Nombre) return true;
			return false;
		};
		auto TieneVector = [&](FName Nombre)
		{
			for (const FCollectionVectorParameter& P : MPC->VectorParameters)
				if (P.ParameterName == Nombre) return true;
			return false;
		};

		bool bCambio = false;
		auto Escalar = [&](const TCHAR* Nombre, float PorDefecto)
		{
			if (TieneEscalar(Nombre)) return;
			FCollectionScalarParameter P;
			P.ParameterName = Nombre; P.DefaultValue = PorDefecto; P.Id = FGuid::NewGuid();
			MPC->ScalarParameters.Add(P); bCambio = true;
		};
		auto Vector = [&](const TCHAR* Nombre, const FLinearColor& PorDefecto)
		{
			if (TieneVector(Nombre)) return;
			FCollectionVectorParameter P;
			P.ParameterName = Nombre; P.DefaultValue = PorDefecto; P.Id = FGuid::NewGuid();
			MPC->VectorParameters.Add(P); bCambio = true;
		};

		// Los dos de siempre: los leen M_Edificio, las fachadas y los materiales
		// que genera UCreadorMaterialesSimples.
		Escalar(TEXT("Wetness"), 0.f);
		Escalar(TEXT("Night"), 0.f);

		// Y los que se escribían contra MPC_AlsasuaGlobal, una colección que no
		// crea nadie. Cuatro sistemas y tres scripts de editor llevaban
		// alimentando parámetros a un null; poner un escalar que no existe sale
		// por un warning y no pasa nada más, así que la capa entera —charcos,
		// ventanas de noche, auto-textura del terreno, viento, grano de
		// película— estaba escribiendo al vacío. Ahora viven donde vive la
		// colección de verdad, que además es la que conduce UClimaSubsystem.
		Escalar(TEXT("GlobalWetness"), 0.f);          // AlsasuaVisualEffectsManager, AlsasuaWorldSubsystem
		Escalar(TEXT("PuddleOpacity"), 0.f);          // SetupEnhancedMaterials.py
		Escalar(TEXT("RainIntensity"), 0.f);
		Escalar(TEXT("SnowAmount"), 0.f);
		Escalar(TEXT("NormalizedTimeOfDay"), 0.f);
		Escalar(TEXT("DayNightBlend"), 0.f);
		Escalar(TEXT("NightEmissiveIntensity"), 0.f); // SetupNightBuildings.py
		Escalar(TEXT("WindIntensity"), 0.f);
		Escalar(TEXT("WindDirection"), 0.f);
		Escalar(TEXT("FogDensityMult"), 1.f);
		Escalar(TEXT("RoadWearAmount"), 0.f);
		Escalar(TEXT("ChromaticAberration"), 0.f);    // AlsasuaCinemaDirector
		Escalar(TEXT("FilmGrain"), 0.f);
		Escalar(TEXT("WPaintRadius"), 0.f);           // AlsasuaRVTHelper
		Escalar(TEXT("WPaintIntensity"), 0.f);

		Vector(TEXT("WindVector"), FLinearColor(0.f, 0.f, 0.f, 0.f));
		Vector(TEXT("WPaintLocation"), FLinearColor(0.f, 0.f, 0.f, 0.f));

		if (bCambio)
		{
			MPC->PostEditChange();
			UEditorAssetLibrary::SaveAsset(Ruta, false);
		}
	}
	return MPC;
}

bool UCreadorMaterialEdificio::CrearMaterialEdificio()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre  = TEXT("M_Edificio");
	const FString Ruta    = Carpeta / Nombre;

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterialParameterCollection* MPC = AsegurarMPCClima(AT);

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	UObject* Obj = AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>());
	UMaterial* Mat = Cast<UMaterial>(Obj);
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Edificio] no pude crear el material")); return false; }

	using ML = UMaterialEditingLibrary;

	// Nodos base.
	UMaterialExpressionVertexColor* VC = Cast<UMaterialExpressionVertexColor>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionVertexColor::StaticClass(), -800, -200));

	// Wetness desde el MPC (0 seco .. 1 mojado).
	UMaterialExpressionCollectionParameter* Wet = Cast<UMaterialExpressionCollectionParameter>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionCollectionParameter::StaticClass(), -800, 250));
	if (Wet && MPC)
	{
		Wet->Collection = MPC; Wet->ParameterName = TEXT("Wetness");
		for (const FCollectionScalarParameter& P : MPC->ScalarParameters)
			if (P.ParameterName == TEXT("Wetness")) { Wet->ParameterId = P.Id; break; }
	}

	// Factor de oscurecimiento: lerp(1.0 seco, 0.55 mojado, Wetness).
	UMaterialExpressionConstant* Uno   = Cast<UMaterialExpressionConstant>(ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -600, 120)); Uno->R = 1.0f;
	UMaterialExpressionConstant* Oscuro= Cast<UMaterialExpressionConstant>(ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -600, 200)); Oscuro->R = 0.55f;
	UMaterialExpressionLinearInterpolate* LerpC = Cast<UMaterialExpressionLinearInterpolate>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), -400, 150));
	ML::ConnectMaterialExpressions(Uno,    TEXT(""), LerpC, TEXT("A"));
	ML::ConnectMaterialExpressions(Oscuro, TEXT(""), LerpC, TEXT("B"));
	ML::ConnectMaterialExpressions(Wet,    TEXT(""), LerpC, TEXT("Alpha"));

	// Tinte = VertexColor * factor de mojado.
	UMaterialExpressionMultiply* Mul = Cast<UMaterialExpressionMultiply>(
		ML::CreateMaterialExpression(Mat, UMaterialExpressionMultiply::StaticClass(), -150, -50));
	ML::ConnectMaterialExpressions(VC,    TEXT(""), Mul, TEXT("A"));
	ML::ConnectMaterialExpressions(LerpC, TEXT(""), Mul, TEXT("B"));

	// Fábrica de ladrillo con el color del barrio como tinte. T_Brick_* estaba
	// descargado y sin usar, y este material genérico de edificio no tenía
	// ninguna textura: color liso, sin junta ni relieve ni AO.
	AlsasuaPBR::FOpciones Op;
	Op.Set = TEXT("Brick");
	Op.TileCm = 200.f;
	Op.RoughnessMojado = 0.3f;
	Op.Tinte = Mul;
	if (!AlsasuaPBR::Cablear(Mat, Op))
	{
		ML::ConnectMaterialProperty(Mul, TEXT(""), MP_BaseColor);

		// Roughness = lerp(0.78 seco, 0.15 mojado, Wetness).
		UMaterialExpressionConstant* RSeco = Cast<UMaterialExpressionConstant>(ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -400, 360)); RSeco->R = 0.78f;
		UMaterialExpressionConstant* RMoj  = Cast<UMaterialExpressionConstant>(ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -400, 440)); RMoj->R = 0.15f;
		UMaterialExpressionLinearInterpolate* LerpR = Cast<UMaterialExpressionLinearInterpolate>(
			ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), -150, 380));
		ML::ConnectMaterialExpressions(RSeco, TEXT(""), LerpR, TEXT("A"));
		ML::ConnectMaterialExpressions(RMoj,  TEXT(""), LerpR, TEXT("B"));
		ML::ConnectMaterialExpressions(Wet,   TEXT(""), LerpR, TEXT("Alpha"));
		ML::ConnectMaterialProperty(LerpR, TEXT(""), MP_Roughness);
	}

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);

	UE_LOG(LogTemp, Log, TEXT("[Edificio] material + MPC_Clima creados en %s"), *Carpeta);
	return true;
}
