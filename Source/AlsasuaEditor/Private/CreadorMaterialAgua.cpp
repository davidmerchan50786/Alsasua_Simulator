// CreadorMaterialAgua.cpp (sólo editor)
#include "CreadorMaterialAgua.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionFresnel.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"
#include "UObject/UnrealType.h"   // FProperty (set por reflexión)

// Fija una propiedad enum (TEnumAsByte) por nombre. Por reflexión: compila
// aunque el campo sea privado en esta versión de UE (5.x ha hecho privados
// varios campos de UMaterial).
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
		P->SetPropertyValue_InContainer(O, Valor);   // soporta bitfields
}

bool UCreadorMaterialAgua::CrearMaterialAgua()
{
	const FString Carpeta = TEXT("/Game/Materiales");
	const FString Nombre  = TEXT("M_AguaRio");
	const FString Ruta    = Carpeta / Nombre;

	if (UEditorAssetLibrary::DoesAssetExist(Ruta))
		UEditorAssetLibrary::DeleteAsset(Ruta);   // sobrescribe

	IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
	UMaterialFactoryNew* Fab = NewObject<UMaterialFactoryNew>();
	UObject* Obj = AT.CreateAsset(Nombre, Carpeta, UMaterial::StaticClass(), Fab);
	UMaterial* Mat = Cast<UMaterial>(Obj);
	if (!Mat) { UE_LOG(LogTemp, Error, TEXT("[Agua] no pude crear el material")); return false; }

	// Translúcido, iluminado. Por reflexión para no depender del acceso C++ de
	// los campos (privados en algunas versiones de UE). SetShadingModel sí es
	// un setter público estable.
	FijarEnumByte(Mat, TEXT("BlendMode"), (uint8)BLEND_Translucent);
	Mat->SetShadingModel(MSM_DefaultLit);
	FijarBool(Mat, TEXT("TwoSided"), true);

	// Color profundo verdoso-azul.
	UMaterialExpressionConstant3Vector* Color = Cast<UMaterialExpressionConstant3Vector>(
		UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionConstant3Vector::StaticClass(), -500, -200));
	Color->Constant = FLinearColor(0.015f, 0.11f, 0.17f);
	UMaterialEditingLibrary::ConnectMaterialProperty(Color, TEXT(""), MP_BaseColor);

	// Muy liso y especular: el agua la "hacen" los reflejos.
	UMaterialExpressionConstant* Rough = Cast<UMaterialExpressionConstant>(
		UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -500, 0));
	Rough->R = 0.03f;
	UMaterialEditingLibrary::ConnectMaterialProperty(Rough, TEXT(""), MP_Roughness);

	UMaterialExpressionConstant* Spec = Cast<UMaterialExpressionConstant>(
		UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -500, 120));
	Spec->R = 1.f;
	UMaterialEditingLibrary::ConnectMaterialProperty(Spec, TEXT(""), MP_Specular);

	// Opacidad por Fresnel: más transparente de frente, más opaca al rasante.
	UMaterialExpressionFresnel* Fr = Cast<UMaterialExpressionFresnel>(
		UMaterialEditingLibrary::CreateMaterialExpression(Mat, UMaterialExpressionFresnel::StaticClass(), -500, 260));
	Fr->Exponent = 4.f;
	Fr->BaseReflectFraction = 0.55f;   // opacidad mínima de frente
	UMaterialEditingLibrary::ConnectMaterialProperty(Fr, TEXT(""), MP_Opacity);

	Mat->PostEditChange();   // propaga los cambios por reflexión (blend/translucidez)
	UMaterialEditingLibrary::RecompileMaterial(Mat);
	UEditorAssetLibrary::SaveAsset(Ruta, false);

	UE_LOG(LogTemp, Log, TEXT("[Agua] material creado en %s"), *Ruta);
	return true;
}
