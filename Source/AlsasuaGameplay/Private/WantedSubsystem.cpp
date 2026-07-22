// WantedSubsystem.cpp
#include "WantedSubsystem.h"

void UWantedSubsystem::AumentarBusqueda(int32 Cantidad)
{
	const int32 Antes = NivelBusqueda;
	NivelBusqueda = FMath::Clamp(NivelBusqueda + Cantidad, 0, 5);
	if (Cantidad > 0) TimerBajar = TiempoBajarNivel;
	if (NivelBusqueda != Antes) OnEstrellasCambia.Broadcast(NivelBusqueda);
}

void UWantedSubsystem::Tick(float DeltaTime)
{
	if (NivelBusqueda <= 0) return;
	TimerBajar -= DeltaTime;
	if (TimerBajar <= 0.f)
	{
		NivelBusqueda = FMath::Max(0, NivelBusqueda - 1);
		TimerBajar = TiempoBajarNivel;
		OnEstrellasCambia.Broadcast(NivelBusqueda);
	}
}
