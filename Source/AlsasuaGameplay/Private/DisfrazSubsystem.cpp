// DisfrazSubsystem.cpp
#include "DisfrazSubsystem.h"
#include "Character/Stealth/GuardDetectionComponent.h"
#include "DiaNocheSubsystem.h"
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
	float Mult = FactorReconocimiento();
	// Night further reduces detection (stacks with disguise).
	if (UWorld* W = GetWorld())
	{
		if (UGameInstance* GI = W->GetGameInstance())
			if (UDiaNocheSubsystem* DN = GI->GetSubsystem<UDiaNocheSubsystem>())
				Mult *= DN->DeteccionSigilo();
	}
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
	// Refresh guard detection every tick to track day/night cycle changes.
	static float RefreshTimer = 0.f;
	RefreshTimer += DeltaTime;
	if (RefreshTimer >= 2.f)
	{
		RefreshTimer = 0.f;
		UpdateGuardDetection();
	}
}
