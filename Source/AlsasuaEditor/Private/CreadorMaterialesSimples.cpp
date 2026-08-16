#include "CreadorMaterialesSimples.h"
#include "CreadorPBRComun.h"
#include "AssetToolsModule.h"
#include "IAssetTools.h"
#include "Factories/MaterialFactoryNew.h"
#include "MaterialEditingLibrary.h"
#include "Materials/Material.h"
#include "Materials/MaterialExpressionConstant.h"
#include "Materials/MaterialExpressionConstant3Vector.h"
#include "Materials/MaterialExpressionLinearInterpolate.h"
#include "EditorAssetLibrary.h"
#include "Modules/ModuleManager.h"

using ML = UMaterialEditingLibrary;

namespace
{
	struct FReceta
	{
		const TCHAR* Carpeta;
		const TCHAR* Nombre;
		/** Set de Content/Textures, o vacío para color liso. */
		const TCHAR* Set;
		FLinearColor Color;
		float TileCm;
		float Roughness;   // sólo si no hay set
		float Metallic;
	};

	// Quién carga cada uno está en el sistema que le da nombre. El color es el
	// que se ve en el pueblo real: el asfalto de Navarra tira a gris azulado, la
	// piedra de Herriko a arena, y las medianeras del casco a ocre.
	const FReceta Recetas[] = {
		// ── Aceras y suelos urbanos (AlsasuaSidewalkSystem, ParkingSystem) ──
		{ TEXT("/Game/Materiales"), TEXT("M_Acera_Hormigon"),   TEXT("Concrete"),    FLinearColor(0.62f, 0.61f, 0.58f), 180.f, 0.75f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Acera_Piedra"),     TEXT("Cobblestone"), FLinearColor(0.55f, 0.52f, 0.47f), 120.f, 0.80f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Pavimento"),        TEXT("Cobblestone"), FLinearColor(0.50f, 0.48f, 0.45f), 140.f, 0.80f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Hormigon"),         TEXT("Concrete"),    FLinearColor(0.58f, 0.58f, 0.56f), 200.f, 0.75f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Hormigon_Garaje"),  TEXT("Concrete"),    FLinearColor(0.45f, 0.45f, 0.44f), 200.f, 0.70f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Suelo_Ciudad"),     TEXT("Concrete"),    FLinearColor(0.52f, 0.51f, 0.49f), 220.f, 0.78f, 0.f },

		// ── Madera y carpintería (DetailDressing, DoorEntrance) ─────────────
		{ TEXT("/Game/Materiales"), TEXT("M_Madera"),           TEXT("Wood"),        FLinearColor(0.42f, 0.28f, 0.16f),  60.f, 0.65f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Puerta"),           TEXT("Wood"),        FLinearColor(0.30f, 0.18f, 0.10f),  50.f, 0.55f, 0.f },

		// ── Metal (OverheadCable, Guardrail, DetailDressing) ────────────────
		{ TEXT("/Game/Materiales"), TEXT("M_Metal_Negro"),      TEXT("MetalPlate"),  FLinearColor(0.06f, 0.06f, 0.07f), 100.f, 0.35f, 1.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Metal_Azul"),       TEXT("MetalPlate"),  FLinearColor(0.10f, 0.20f, 0.45f), 100.f, 0.35f, 1.f },
		// El quitamiedos de carretera es acero galvanizado, no pintado.
		{ TEXT("/Game/Materiales"), TEXT("M_Metal_Guardia"),    TEXT("MetalPlate"),  FLinearColor(0.62f, 0.64f, 0.66f), 120.f, 0.30f, 1.f },

		// ── Piedra y vegetación (DetailDressing) ────────────────────────────
		{ TEXT("/Game/Materiales"), TEXT("M_Piedra"),           TEXT("StoneWall"),   FLinearColor(0.52f, 0.49f, 0.44f), 150.f, 0.85f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Seto"),             TEXT("Grass"),       FLinearColor(0.16f, 0.30f, 0.12f),  40.f, 0.90f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Verde_Oscuro"),     TEXT("Grass"),       FLinearColor(0.12f, 0.24f, 0.10f),  40.f, 0.90f, 0.f },

		// ── Pintado sobre pared (StreetArt, PaintedStreetSign, ShopFront) ───
		// Van sobre hormigón porque eso es lo que hay debajo: el tinte es la
		// pintura y el set aporta el poro del muro, que es lo que hace que un
		// grafiti no parezca una calcomanía.
		{ TEXT("/Game/Materiales"), TEXT("M_Grafiti"),          TEXT("Concrete"),    FLinearColor(0.75f, 0.20f, 0.18f), 160.f, 0.70f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Mural_Pared"),      TEXT("Concrete"),    FLinearColor(0.40f, 0.45f, 0.55f), 160.f, 0.70f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Rotulo_Pared"),     TEXT("Concrete"),    FLinearColor(0.85f, 0.84f, 0.80f), 160.f, 0.65f, 0.f },
		{ TEXT("/Game/Materiales"), TEXT("M_Tienda"),           TEXT("Concrete"),    FLinearColor(0.70f, 0.66f, 0.60f), 150.f, 0.60f, 0.f },

		// ── Toldo: lona, sin set propio en Content/Textures ─────────────────
		{ TEXT("/Game/Materiales"), TEXT("M_Toldo"),            TEXT(""),            FLinearColor(0.55f, 0.15f, 0.14f),   0.f, 0.85f, 0.f },

		// ── Vehículos (DynamicTrafficSystem) ────────────────────────────────
		{ TEXT("/Game/Materiales"), TEXT("M_Vehiculo_Base"),    TEXT("MetalPlate"),  FLinearColor(0.20f, 0.21f, 0.24f), 200.f, 0.25f, 0.9f },
		{ TEXT("/Game/Materiales"), TEXT("M_Vehiculo_Blanco"),  TEXT("MetalPlate"),  FLinearColor(0.82f, 0.83f, 0.84f), 200.f, 0.25f, 0.9f },

		// ── Firme por barrio (AlsasuaTerrainLayersSystem) ───────────────────
		// Cinco barrios y la playa de vías, cada uno con su firme. El sistema los
		// pide por ruta y ninguno existía, así que las seis capas de terreno por
		// barrio no se veían.
		{ TEXT("/Game/Materiales/Pavimentos"), TEXT("M_Asphalt_Zelai"),       TEXT("Asphalt"), FLinearColor(0.16f, 0.16f, 0.17f), 240.f, 0.72f, 0.f },
		{ TEXT("/Game/Materiales/Pavimentos"), TEXT("M_Asphalt_Intxostia"),   TEXT("Asphalt"), FLinearColor(0.18f, 0.18f, 0.19f), 240.f, 0.72f, 0.f },
		{ TEXT("/Game/Materiales/Pavimentos"), TEXT("M_Asphalt_SanPedro"),    TEXT("Asphalt"), FLinearColor(0.15f, 0.15f, 0.16f), 240.f, 0.72f, 0.f },
		// La playa de vías está más sucia de balasto y aceite que la calle.
		{ TEXT("/Game/Materiales/Pavimentos"), TEXT("M_Asphalt_Ferroviario"), TEXT("Asphalt"), FLinearColor(0.12f, 0.12f, 0.12f), 260.f, 0.80f, 0.f },
		{ TEXT("/Game/Materiales/Pavimentos"), TEXT("M_Gravel_Errota"),       TEXT("Ground"),  FLinearColor(0.48f, 0.44f, 0.38f), 160.f, 0.88f, 0.f },
		{ TEXT("/Game/Materiales/Pavimentos"), TEXT("M_Piedra_Herriko"),      TEXT("Cobblestone"), FLinearColor(0.56f, 0.52f, 0.45f), 110.f, 0.82f, 0.f },
		{ TEXT("/Game/Materiales/Pavimentos"), TEXT("M_Piedra_Harrobieta"),   TEXT("Cobblestone"), FLinearColor(0.50f, 0.47f, 0.43f), 110.f, 0.82f, 0.f },
		{ TEXT("/Game/Materiales/Pavimentos"), TEXT("M_Tierra_Monte"),        TEXT("Ground"),  FLinearColor(0.38f, 0.31f, 0.22f), 180.f, 0.92f, 0.f },
	};

	bool CrearUno(const FReceta& R)
	{
		const FString Ruta = FString(R.Carpeta) / R.Nombre;
		if (UEditorAssetLibrary::DoesAssetExist(Ruta))
			UEditorAssetLibrary::DeleteAsset(Ruta);

		IAssetTools& AT = FModuleManager::LoadModuleChecked<FAssetToolsModule>("AssetTools").Get();
		UMaterial* Mat = Cast<UMaterial>(AT.CreateAsset(R.Nombre, R.Carpeta,
			UMaterial::StaticClass(), NewObject<UMaterialFactoryNew>()));
		if (!Mat) return false;

		auto* Tinte = Cast<UMaterialExpressionConstant3Vector>(
			ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant3Vector::StaticClass(), -900, -100));
		Tinte->Constant = R.Color;

		bool bConSet = false;
		if (FString(R.Set).Len() > 0)
		{
			AlsasuaPBR::FOpciones Op;
			Op.Set = R.Set;
			Op.TileCm = R.TileCm;
			Op.Tinte = Tinte;
			bConSet = AlsasuaPBR::Cablear(Mat, Op);
		}

		if (!bConSet)
		{
			// Sin set descargado: color liso con su rugosidad, y aun así
			// respondiendo a la lluvia como el resto del pueblo.
			ML::ConnectMaterialProperty(Tinte, TEXT(""), MP_BaseColor);

			auto Const = [&](float v, int32 y)
			{
				auto* c = Cast<UMaterialExpressionConstant>(
					ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -700, y));
				c->R = v;
				return (UMaterialExpression*)c;
			};

			UMaterialExpression* Rough = Const(R.Roughness, 100);
			if (UMaterialExpression* Wet = AlsasuaPBR::Clima(Mat, TEXT("Wetness"), -700, 140))
			{
				auto* Lerp = Cast<UMaterialExpressionLinearInterpolate>(
					ML::CreateMaterialExpression(Mat, UMaterialExpressionLinearInterpolate::StaticClass(), -450, 180));
				ML::ConnectMaterialExpressions(Const(R.Roughness, 160), TEXT(""), Lerp, TEXT("A"));
				ML::ConnectMaterialExpressions(Const(0.15f, 200), TEXT(""), Lerp, TEXT("B"));
				ML::ConnectMaterialExpressions(Wet, TEXT(""), Lerp, TEXT("Alpha"));
				Rough = Lerp;
			}
			ML::ConnectMaterialProperty(Rough, TEXT(""), MP_Roughness);
		}

		if (R.Metallic > 0.f)
		{
			auto* Met = Cast<UMaterialExpressionConstant>(
				ML::CreateMaterialExpression(Mat, UMaterialExpressionConstant::StaticClass(), -700, 260));
			Met->R = R.Metallic;
			ML::ConnectMaterialProperty(Met, TEXT(""), MP_Metallic);
		}

		Mat->PostEditChange();
		ML::RecompileMaterial(Mat);
		UEditorAssetLibrary::SaveAsset(Ruta, false);
		return true;
	}
}

int32 UCreadorMaterialesSimples::CrearMaterialesSueltos()
{
	int32 Hechos = 0;
	for (const FReceta& R : Recetas)
	{
		if (CrearUno(R)) ++Hechos;
		else UE_LOG(LogTemp, Warning, TEXT("[Materiales] no pude crear %s/%s"), R.Carpeta, R.Nombre);
	}
	UE_LOG(LogTemp, Log, TEXT("[Materiales] %d de %d materiales sueltos creados."),
		Hechos, (int32)UE_ARRAY_COUNT(Recetas));
	return Hechos;
}

bool UCreadorMaterialesSimples::CrearSueltos()
{
	return CrearMaterialesSueltos() == (int32)UE_ARRAY_COUNT(Recetas);
}
