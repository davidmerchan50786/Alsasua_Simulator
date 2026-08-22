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

AlsasuaDirecciones::FFachada AlsasuaDirecciones::LadoDeEntrada(int32 IdEdificio,
	const FVector2D& Min2, const FVector2D& Max2, FRandomStream& Sorteo)
{
	const FVector2D Centro = (Min2 + Max2) * 0.5f;

	// Los cuatro lados de la caja, con el yaw que mira afuera.
	const FVector2D Lados[4] = {
		FVector2D(Max2.X, Centro.Y), FVector2D(Min2.X, Centro.Y),
		FVector2D(Centro.X, Max2.Y), FVector2D(Centro.X, Min2.Y) };
	const float Yaws[4] = { 0.0f, 180.0f, 90.0f, 270.0f };
	const FVector2D Fueras[4] = {
		FVector2D(1.0f, 0.0f), FVector2D(-1.0f, 0.0f),
		FVector2D(0.0f, 1.0f), FVector2D(0.0f, -1.0f) };

	const FDireccion* Dir = De(IdEdificio);

	FFachada F;
	int32 Lado = 0;
	if (Dir && Dir->bTienePuntoCalle)
	{
		float MejorDist2 = TNumericLimits<float>::Max();
		for (int32 i = 0; i < 4; ++i)
		{
			const float D2 = FVector2D::DistSquared(Lados[i], Dir->PuntoCalle);
			if (D2 < MejorDist2) { MejorDist2 = D2; Lado = i; }
		}
		F.bHaciaCalle = true;
	}
	else
	{
		// Sin calle conocida: el lado largo, con el sentido sorteado por id.
		const bool bLadoEnX = (Max2.X - Min2.X) >= (Max2.Y - Min2.Y);
		Lado = (bLadoEnX ? 0 : 2) + (Sorteo.GetFraction() < 0.5f ? 0 : 1);
	}

	F.Punto = Lados[Lado];
	F.Yaw = Yaws[Lado];
	F.Fuera = Fueras[Lado];
	return F;
}
