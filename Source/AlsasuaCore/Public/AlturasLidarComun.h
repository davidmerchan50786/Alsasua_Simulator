// AlturasLidarComun.h (capa CORE)
// Alturas de edificio medidas por LiDAR, compartidas por quien las necesite.
//
// Las alturas de buildings_final.json y de building_facades.json vienen de OSM y
// se quedan cortas: la mediana del mundo era 7,03 m contra los 9,50 m que mide
// el vuelo LiDAR de Navarra 2017 sobre las mismas huellas, casi una planta menos
// en todo el pueblo. Con esto cada edificio usa su altura real medida.
//
// Vive en Core y no en el módulo que la usa porque la necesitan dos: el cargador
// de edificios (para el volumen) y el generador de fachadas (para cuántas filas
// de ventanas caben). Si sólo la usara uno, el otro dibujaría con la altura vieja
// y quedaría muro desnudo por encima de la última ventana.
//
// El dato lo genera Tools/AlturasLidarEdificios.py. Si el fichero no está, Buscar
// devuelve false siempre y cada sistema se queda con lo que diga su JSON: el
// proyecto sigue arrancando igual, sólo que con las alturas estimadas de antes.
#pragma once

#include "CoreMinimal.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace AlturasLidar
{
	struct FEntrada
	{
		FVector2D Centro = FVector2D::ZeroVector;   // cm de mundo
		float AlturaM = 0.f;
		int32 Plantas = 0;
	};

	/** Lado de celda de la rejilla de búsqueda (cm). */
	inline constexpr float CeldaCm = 3000.f;
	/** Radio máximo para dar por buena una pareja huella-edificio (cm). */
	inline constexpr float RadioMatchCm = 1500.f;

	inline TMap<FIntPoint, TArray<FEntrada>>& Rejilla()
	{
		static TMap<FIntPoint, TArray<FEntrada>> R;
		return R;
	}

	inline void Cargar()
	{
		static bool bHecho = false;
		if (bHecho) return;
		bHecho = true;

		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(),
			TEXT("Datos/alturas_lidar_edificios.json"));
		FString Texto;
		if (!FFileHelper::LoadFileToString(Texto, *Ruta))
		{
			UE_LOG(LogTemp, Log, TEXT("[AlturasLidar] sin %s; se usan las alturas del JSON."), *Ruta);
			return;
		}

		TSharedPtr<FJsonObject> Doc;
		const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
		if (!FJsonSerializer::Deserialize(R, Doc) || !Doc.IsValid()) return;

		const TArray<TSharedPtr<FJsonValue>>* Arr = nullptr;
		if (!Doc->TryGetArrayField(TEXT("edificios"), Arr) || !Arr) return;

		int32 N = 0;
		for (const TSharedPtr<FJsonValue>& V : *Arr)
		{
			const TSharedPtr<FJsonObject> O = V->AsObject();
			if (!O.IsValid()) continue;
			const TArray<TSharedPtr<FJsonValue>>* C = nullptr;
			if (!O->TryGetArrayField(TEXT("centro_mundo_cm"), C) || !C || C->Num() < 2) continue;

			FEntrada E;
			E.Centro = FVector2D((*C)[0]->AsNumber(), (*C)[1]->AsNumber());
			E.AlturaM = (float)O->GetNumberField(TEXT("altura_m"));
			E.Plantas = (int32)O->GetNumberField(TEXT("plantas"));
			if (E.Plantas <= 0) continue;   // cobertizos y ruido: no sustituyen nada

			Rejilla().FindOrAdd(FIntPoint(FMath::FloorToInt(E.Centro.X / CeldaCm),
			                              FMath::FloorToInt(E.Centro.Y / CeldaCm))).Add(E);
			++N;
		}
		UE_LOG(LogTemp, Log, TEXT("[AlturasLidar] %d huellas con altura medida."), N);
	}

	/** Altura (m) y plantas medidas cerca de ese centro de mundo (cm). */
	inline bool Buscar(const FVector2D& CentroCm, float& OutAlturaM, int32& OutPlantas)
	{
		Cargar();
		const FIntPoint C(FMath::FloorToInt(CentroCm.X / CeldaCm),
		                  FMath::FloorToInt(CentroCm.Y / CeldaCm));
		float MejorD2 = RadioMatchCm * RadioMatchCm;
		const FEntrada* Mejor = nullptr;
		for (int32 dx = -1; dx <= 1; ++dx)
		{
			for (int32 dy = -1; dy <= 1; ++dy)
			{
				if (const TArray<FEntrada>* L = Rejilla().Find(FIntPoint(C.X + dx, C.Y + dy)))
				{
					for (const FEntrada& E : *L)
					{
						const float D2 = FVector2D::DistSquared(E.Centro, CentroCm);
						if (D2 < MejorD2) { MejorD2 = D2; Mejor = &E; }
					}
				}
			}
		}
		if (!Mejor) return false;
		OutAlturaM = Mejor->AlturaM;
		OutPlantas = Mejor->Plantas;
		return true;
	}
}
