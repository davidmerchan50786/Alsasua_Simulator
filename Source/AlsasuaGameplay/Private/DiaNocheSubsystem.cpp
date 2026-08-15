// DiaNocheSubsystem.cpp
#include "DiaNocheSubsystem.h"
#include "World/Time/TimeOfDayManager.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"

float UDiaNocheSubsystem::FactorIngresoNegocio(ETipoNegocio Tipo) const
{
	switch (Tipo)
	{
	case ETipoNegocio::Bar:       return EsNoche() ? 1.6f : 0.6f;   // el bar es de noche
	case ETipoNegocio::Comercio:  return EsDia()   ? 1.3f : 0.4f;
	case ETipoNegocio::Empresa:   return EsDia()   ? 1.2f : 0.3f;
	case ETipoNegocio::Industria: return EsDia()   ? 1.2f : 0.3f;
	default:                      return 1.f;
	}
}

void UDiaNocheSubsystem::Tick(float DeltaTime)
{
	// El reloj de gameplay y el que mueve el sol tienen que ser el mismo: si no,
	// el alumbrado y las ventanas se encienden a una hora y el sol está en otra.
	// UTimeOfDayManager manda cuando existe; este reloj es su espejo.
	if (const UWorld* W = GetGameInstance() ? GetGameInstance()->GetWorld() : nullptr)
	{
		if (const UTimeOfDayManager* Tod = W->GetSubsystem<UTimeOfDayManager>())
		{
			Hora = Tod->CurrentTime;
			return;
		}
	}

	Hora = FMath::Fmod(Hora + DeltaTime * HorasPorSegundo, 24.f);
}
