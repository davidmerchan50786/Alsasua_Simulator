// DisfrazSubsystem.cpp
#include "DisfrazSubsystem.h"

void UDisfrazSubsystem::Alternar()
{
	if (bEncubierto) { bEncubierto = false; return; }
	if (Cooldown > 0.f) return;       // aún te recuerdan
	bEncubierto = true;
}

void UDisfrazSubsystem::Delatar()
{
	if (!bEncubierto) return;
	bEncubierto = false;
	Cooldown = 8.f;                   // enfriamiento tras delatarte
}

void UDisfrazSubsystem::Tick(float DeltaTime)
{
	if (Cooldown > 0.f) Cooldown -= DeltaTime;
}
