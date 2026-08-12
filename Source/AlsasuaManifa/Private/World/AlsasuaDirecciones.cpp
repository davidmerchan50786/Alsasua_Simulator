// AlsasuaDirecciones.cpp
#include "World/AlsasuaDirecciones.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	TMap<int32, AlsasuaDirecciones::FDireccion> Direcciones;
	bool bLeidas = false;

	void Cargar()
	{
		if (bLeidas) return;
		bLeidas = true;

		const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/direcciones_osm.json"));
		FString Texto;
		if (!FFileHelper::LoadFileToString(Texto, *Ruta))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Direcciones] no pude leer %s"), *Ruta);
			return;
		}

		TArray<TSharedPtr<FJsonValue>> Items;
		const TSharedRef<TJsonReader<>> Lector = TJsonReaderFactory<>::Create(Texto);
		if (!FJsonSerializer::Deserialize(Lector, Items))
		{
			UE_LOG(LogTemp, Warning, TEXT("[Direcciones] JSON inválido en %s"), *Ruta);
			return;
		}

		int32 ConPunto = 0;
		for (const TSharedPtr<FJsonValue>& Valor : Items)
		{
			const TSharedPtr<FJsonObject> Obj = Valor->AsObject();
			if (!Obj.IsValid() || !Obj->HasField(TEXT("id"))) continue;

			AlsasuaDirecciones::FDireccion D;
			Obj->TryGetStringField(TEXT("calle"), D.Calle);
			Obj->TryGetStringField(TEXT("portal"), D.Portal);

			// calle_x/calle_z son números en el JSON, no cadenas: leerlos con
			// TryGetStringField devolvería false para los 374 y la puerta nunca
			// encontraría su calle, sin una sola queja en el log.
			double X = 0.0, Z = 0.0;
			if (Obj->TryGetNumberField(TEXT("calle_x"), X) && Obj->TryGetNumberField(TEXT("calle_z"), Z))
			{
				D.PuntoCalle = FVector2D(X, Z);
				D.bTienePuntoCalle = true;
				++ConPunto;
			}

			Direcciones.Add((int32)Obj->GetIntegerField(TEXT("id")), MoveTemp(D));
		}

		UE_LOG(LogTemp, Log, TEXT("[Direcciones] %d edificios con dirección, %d con punto de calle"),
			Direcciones.Num(), ConPunto);
	}
}

const AlsasuaDirecciones::FDireccion* AlsasuaDirecciones::De(int32 IdEdificio)
{
	Cargar();
	return Direcciones.Find(IdEdificio);
}
