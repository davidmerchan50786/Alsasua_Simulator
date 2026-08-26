// DisfrazSubsystem.cpp
#include "DisfrazSubsystem.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "EngineUtils.h"
#include "Engine/World.h"

void UDisfrazSubsystem::Alternar()
{
	if (bEncubierto) { bEncubierto = false; UpdateGuardDetection(); return; }
	if (Cooldown > 0.f) return;
	bEncubierto = true;
	UpdateGuardDetection();
}

void UDisfrazSubsystem::Delatar()
{
	if (!bEncubierto) return;
	bEncubierto = false;
	Cooldown = 8.f;
	UpdateGuardDetection();
}

void UDisfrazSubsystem::UpdateGuardDetection()
{
	const float Mult = FactorReconocimiento();
	if (UWorld* W = GetWorld())
	{
		for (TActorIterator<AActor> It(W); It; ++It)
		{
			if (UGuardDetectionComponent* Det = It->FindComponentByClass<UGuardDetectionComponent>())
				Det->DetectionRangeMultiplier = Mult;
		}
	}
}

void UDisfrazSubsystem::Tick(float DeltaTime)
{
	if (Cooldown > 0.f) Cooldown -= DeltaTime;
}
