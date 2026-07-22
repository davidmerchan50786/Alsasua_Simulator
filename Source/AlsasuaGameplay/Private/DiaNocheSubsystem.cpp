// DiaNocheSubsystem.cpp
#include "DiaNocheSubsystem.h"

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
	Hora = FMath::Fmod(Hora + DeltaTime * HorasPorSegundo, 24.f);
}
