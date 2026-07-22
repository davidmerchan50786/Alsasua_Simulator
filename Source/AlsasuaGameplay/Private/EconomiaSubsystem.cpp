// EconomiaSubsystem.cpp
#include "EconomiaSubsystem.h"

void UEconomiaSubsystem::GanarDinero(int32 Cantidad)
{
	Dinero += Cantidad;
	Puntuacion += Cantidad;
	OnEconomiaCambia.Broadcast(Dinero, Puntuacion);
}

bool UEconomiaSubsystem::GastarDinero(int32 Cantidad)
{
	if (Dinero < Cantidad) return false;
	Dinero -= Cantidad;
	OnEconomiaCambia.Broadcast(Dinero, Puntuacion);
	return true;
}
