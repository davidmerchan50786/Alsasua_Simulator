#include "CreadorMaterialCalles.h"
#include "CreadorPBRComun.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

/** Crea un material de suelo con el set PBR completo del nombre indicado. */
static void CrearSuelo(const FString& Nombre, const FString& Set, float TileCm,
	float RoughnessMojado, const FString& Categoria)
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Ruta = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[%s] no pude crear el material"), *Categoria); return; }

	AlsasuaPBR::FOpciones Op;
	Op.Set = Set;
	Op.TileCm = TileCm;
	Op.RoughnessMojado = RoughnessMojado;
	AlsasuaPBR::Cablear(Mat, Op);

	Mat->PostEditChange();
	ML::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);
	UE_LOG(LogTemp, Log, TEXT("[%s] material creado en %s"), *Categoria, *Ruta);
}

bool UCreadorMaterialCalles::CrearMaterialCalles()
{
	// Asfalto: teselado 2 m. El mapa de roughness trae las rodadas pulidas y el
	// árido suelto; antes era 0.7 plano para toda la calzada.
	// Mojado baja a 0.08: el asfalto encharcado es casi un espejo.
	CrearSuelo(TEXT("M_Terreno_Calles"), TEXT("Asphalt"), 200.f, 0.08f, TEXT("Calles"));
	return true;
}

bool UCreadorMaterialCalles::CrearMaterialAcera()
{
	// Adoquín: teselado 0.5 m. Ahora también con normal y AO, que estaban
	// descargados y sin usar (la acera sólo tenía color).
	CrearSuelo(TEXT("M_Terreno_Acera"), TEXT("Cobblestone"), 50.f, 0.18f, TEXT("Acera"));
	return true;
}

bool UCreadorMaterialCalles::CrearMaterialHierba()
{
	// Césped: teselado 1 m. Mojado apenas brilla, la hierba absorbe el agua.
	CrearSuelo(TEXT("M_Terreno_Hierba"), TEXT("Grass"), 100.f, 0.55f, TEXT("Hierba"));
	return true;
}

bool UCreadorMaterialCalles::CrearMaterialTierra()
{
	// Tierra y grava de sendas: teselado 1.5 m. Encharcada sí refleja.
	CrearSuelo(TEXT("M_Terreno_Tierra"), TEXT("Ground"), 150.f, 0.2f, TEXT("Tierra"));
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
