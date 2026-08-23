// ContratosClima.h - Contratos del pilar clima, sin el pilar.
//
// Quien consume el clima no puede incluir cabeceras de GF_Clima: eso clava la
// dependencia de compilacion y el dia que el plugin duerme (o se descarga en
// runtime), el consumidor no compila o crashea. Aqui vive SOLO la interfaz:
// GF_Clima la implementa y publica su implementacion en UAlsasuaServiceRegistry
// ("Clima.TiempoDelDia", "Clima.Meteorologia"); los consumidores preguntan por
// nombre con PedirComo<> y reciben nullptr si nadie publico.
//
// Metodos plain virtual, sin UFUNCTION a proposito: los consumidores son C++,
// y UFUNCTION aqui obligaria a exponer tipos concretos que el contrato debe
// ignorar.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "ContratosClima.generated.h"

UINTERFACE(BlueprintType)
class UTimeOfDayService : public UInterface
{
	GENERATED_BODY()
};

class ALSASUACONTRACTS_API ITimeOfDayService
{
	GENERATED_BODY()

public:
	/** Elevacion solar en grados; negativa de noche. */
	virtual float GetSunPitch() const = 0;

	/** Hora local del mundo simulado (0-24). */
	virtual float GetHour() const = 0;

	virtual bool IsNight() const = 0;

	/** Direccion normalizada HACIA el sol desde la escena. */
	virtual FVector GetSunDirection() const = 0;

	virtual FLinearColor GetSunColor() const = 0;
	virtual float GetSunIntensity() const = 0;
};

UINTERFACE(BlueprintType)
class UWeatherService : public UInterface
{
	GENERATED_BODY()
};

/** Vocabulario meteorologico del contrato; los consumidores comparan contra
 *  esto, nunca contra el enum interno del plugin. */
UENUM(BlueprintType)
enum class EAlsasuaWeatherState : uint8 {
	Clear,
	Rainy,
	HeavyFog,
	Thunderstorm
};

class ALSASUACONTRACTS_API IWeatherService
{
	GENERATED_BODY()

public:
	/** Estado discreto actual; para logica que ramifica por tiempo. */
	virtual EAlsasuaWeatherState GetWeatherState() const = 0;

	// Impacto en jugabilidad: 1 = condiciones despejadas.
	virtual float GetTireGripMultiplier() const = 0;
	virtual float GetAIVisibilityMultiplier() const = 0;
	virtual float GetFootstepNoiseMultiplier() const = 0;

	/** 0 = seco, 1 = tormenta. */
	virtual float GetRainIntensity() const = 0;

	/** km/h. */
	virtual float GetWindSpeedKmh() const = 0;

	/** Grados Celsius aparentes segun estado meteorologico. */
	virtual float GetTemperatureCelsius() const = 0;
};
