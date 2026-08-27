// WantedSubsystem.cpp
#include "WantedSubsystem.h"
#include "DiaNocheSubsystem.h"

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

	// Police are slower to deescalate at night (1.5x timer).
	float TimeMult = 1.f;
	if (UGameInstance* GI = GetGameInstance())
		if (UDiaNocheSubsystem* DN = GI->GetSubsystem<UDiaNocheSubsystem>())
			if (DN->EsNoche()) TimeMult = 1.5f;

	TimerBajar -= DeltaTime / TimeMult;
	if (TimerBajar <= 0.f)
	{
		NivelBusqueda = FMath::Max(0, NivelBusqueda - 1);
		TimerBajar = TiempoBajarNivel;
		OnEstrellasCambia.Broadcast(NivelBusqueda);
	}
}
