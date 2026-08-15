// CargarJsonComun.h (capa CORE)
// Carga un dataset de Content/Datos/ como array de objetos, sea cual sea la
// forma de su raíz.
//
// Los 30 ficheros de Content/Datos/ no comparten forma: unos son un array en la
// raíz (roads_unity.json, trees_unity.json, street_furniture.json...) y otros
// envuelven el array en un objeto (railways_unity.json en "rails",
// poi_data.json en "pois", nighborhoods.json en "barrios"). Los generó gente
// distinta en momentos distintos y nadie unificó nada.
//
// Eso, por sí solo, no sería un problema. Lo que sí lo es: en UE, deserializar a
// TArray contra una raíz de objeto —o a FJsonObject contra una raíz de array—
// devuelve false y ya está. Sin excepción, sin aviso, sin nada en el log. El
// sistema que lo lee hace `return` y sigue arrancando como si tal cosa,
// registrando incluso que ha cargado. Así se perdieron, cada uno por su lado y
// sin que nadie se enterara, la vía férrea entera, las superficies de calle, los
// coches aparcados, las señales de tráfico y las calles del minimapa.
//
// Por eso esto vive en Core y no en cada sistema: la forma de la raíz no es
// asunto del que consume el dato. Se le pide el array, y si el fichero no tiene
// nada aprovechable se entera por el log en vez de quedarse mudo.
#pragma once

#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace JsonDatos
{
	/**
	 * Lee Content/<RutaRel> y deja en Out el array de objetos que contenga.
	 *
	 * @param RutaRel        Relativa a Content, p.ej. TEXT("Datos/roads_unity.json").
	 * @param Out            Array resultante. Se vacía siempre antes de escribir.
	 * @param CamposPosibles Nombres de campo a probar si la raíz es un objeto, en
	 *                       orden. Si ninguno casa se coge el primer campo que
	 *                       sea un array y se dice cuál en el log: un dataset
	 *                       regenerado con otro nombre de envoltorio sigue
	 *                       funcionando, pero deja constancia.
	 * @return true si Out tiene al menos un elemento.
	 */
	inline bool CargarArray(const FString& RutaRel,
	                        TArray<TSharedPtr<FJsonValue>>& Out,
	                        const TArray<FString>& CamposPosibles = TArray<FString>())
	{
		Out.Reset();

		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), RutaRel);
		FString Texto;
		if (!FFileHelper::LoadFileToString(Texto, *Ruta))
		{
			UE_LOG(LogTemp, Warning, TEXT("[JsonDatos] no existe %s"), *Ruta);
			return false;
		}

		// Primero como array. Es la forma más común y el caso barato.
		{
			const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
			if (FJsonSerializer::Deserialize(R, Out) && Out.Num() > 0)
			{
				return true;
			}
			Out.Reset();   // un intento fallido puede dejarlo a medias
		}

		// Si no, como objeto que envuelve el array.
		TSharedPtr<FJsonObject> Doc;
		const TSharedRef<TJsonReader<>> R2 = TJsonReaderFactory<>::Create(Texto);
		if (!FJsonSerializer::Deserialize(R2, Doc) || !Doc.IsValid())
		{
			UE_LOG(LogTemp, Warning, TEXT("[JsonDatos] %s no es ni array ni objeto"), *Ruta);
			return false;
		}

		const TArray<TSharedPtr<FJsonValue>>* Campo = nullptr;
		for (const FString& Nombre : CamposPosibles)
		{
			if (Doc->TryGetArrayField(Nombre, Campo) && Campo && Campo->Num() > 0)
			{
				Out = *Campo;
				return true;
			}
		}

		// Ningún nombre esperado: se coge el primer campo que sea un array no
		// vacío y se canta cuál, que es mejor que devolver nada en silencio.
		for (const auto& Par : Doc->Values)
		{
			if (!Par.Value.IsValid() || Par.Value->Type != EJson::Array) continue;
			const TArray<TSharedPtr<FJsonValue>>& Arr = Par.Value->AsArray();
			if (Arr.Num() == 0) continue;

			UE_LOG(LogTemp, Warning,
				TEXT("[JsonDatos] %s: ningún campo esperado; se usa \"%s\" (%d elementos)."),
				*RutaRel, *Par.Key, Arr.Num());
			Out = Arr;
			return true;
		}

		UE_LOG(LogTemp, Warning, TEXT("[JsonDatos] %s no tiene ningún array con datos."), *RutaRel);
		return false;
	}
}
