#include "World/AlsasuaAmbientParticles.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "World/Time/TimeOfDayManager.h"
#include "World/Weather/WeatherSubsystem.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"

UAlsasuaAmbientParticles::UAlsasuaAmbientParticles()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.15f;
}

void UAlsasuaAmbientParticles::BeginPlay()
{
	Super::BeginPlay();
}

void UAlsasuaAmbientParticles::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateAmbientParticles(DeltaTime);
}

void UAlsasuaAmbientParticles::UpdateAmbientParticles(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0);
	if (!PC || !PC->GetPawn()) return;

	APawn* Pawn = PC->GetPawn();
	const FVector PawnLoc = Pawn->GetActorLocation();

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	UTimeOfDayManager* TimeMgr = W->GetSubsystem<UTimeOfDayManager>();
	UWeatherSubsystem* Weather = W->GetSubsystem<UWeatherSubsystem>();

	const float Wind = VFXMgr ? VFXMgr->WindIntensity : 0.3f;
	const float Hour = TimeMgr ? TimeMgr->CurrentTime : 12.f;
	const float Season = TimeMgr ? TimeMgr->CurrentTime / 365.f : 0.5f;
	const bool bRaining = Weather && (Weather->CurrentWeather == EWeatherSubsystemState::Rainy ||
		Weather->CurrentWeather == EWeatherSubsystemState::Thunderstorm);

	TimeAccum += DeltaTime;

	// --- Dust ---
	if (bEnableDust && !bRaining)
	{
		if (!ActiveDustSystem)
		{
			UNiagaraSystem* DustNS = LoadObject<UNiagaraSystem>(nullptr,
				TEXT("/Game/Effects/NS_Dust.NS_Dust"));
			if (DustNS)
			{
				ActiveDustSystem = UNiagaraFunctionLibrary::SpawnSystemAttached(
					DustNS, Pawn->GetRootComponent(), NAME_None,
					FVector::ZeroVector, FRotator::ZeroRotator,
					EAttachLocation::KeepRelativeOffset, true);
			}
		}

		if (ActiveDustSystem)
		{
			const float Rate = DustSpawnRate * (1.f + Wind * DustWindSensitivity);
			ActiveDustSystem->SetVariableFloat(FName("SpawnRate"), Rate);
			ActiveDustSystem->SetVariableFloat(FName("WindStrength"), Wind);
			ActiveDustSystem->SetVectorParameter(FName("EmitterOrigin"),
				PawnLoc + FVector(0, 0, 50.f));
		}
	}
	else if (ActiveDustSystem)
	{
		ActiveDustSystem->Deactivate();
		ActiveDustSystem = nullptr;
	}

	// --- Pollen ---
	if (bEnablePollen && !bRaining && Season > PollenSeasonStart && Season < PollenSeasonEnd)
	{
		if (!ActivePollenSystem)
		{
			UNiagaraSystem* PollenNS = LoadObject<UNiagaraSystem>(nullptr,
				TEXT("/Game/Effects/NS_Pollen.NS_Pollen"));
			if (PollenNS)
			{
				ActivePollenSystem = UNiagaraFunctionLibrary::SpawnSystemAttached(
					PollenNS, Pawn->GetRootComponent(), NAME_None,
					FVector::ZeroVector, FRotator::ZeroRotator,
					EAttachLocation::KeepRelativeOffset, true);
			}
		}

		if (ActivePollenSystem)
		{
			ActivePollenSystem->SetVariableFloat(FName("SpawnRate"), PollenSpawnRate);
			ActivePollenSystem->SetVariableFloat(FName("WindStrength"), Wind);
			ActivePollenSystem->SetVectorParameter(FName("EmitterOrigin"), PawnLoc);
		}
	}
	else if (ActivePollenSystem)
	{
		ActivePollenSystem->Deactivate();
		ActivePollenSystem = nullptr;
	}

	// --- Falling Leaves ---
	if (bEnableLeaves && !bRaining && Season > AutumnStart && Season < AutumnEnd)
	{
		if (!ActiveLeafSystem)
		{
			UNiagaraSystem* LeafNS = LoadObject<UNiagaraSystem>(nullptr,
				TEXT("/Game/Effects/NS_Leaves.NS_Leaves"));
			if (LeafNS)
			{
				ActiveLeafSystem = UNiagaraFunctionLibrary::SpawnSystemAttached(
					LeafNS, Pawn->GetRootComponent(), NAME_None,
					FVector::ZeroVector, FRotator::ZeroRotator,
					EAttachLocation::KeepRelativeOffset, true);
			}
		}

		if (ActiveLeafSystem)
		{
			ActiveLeafSystem->SetVariableFloat(FName("SpawnRate"), LeafSpawnRate);
			ActiveLeafSystem->SetVariableFloat(FName("WindStrength"), Wind);
			ActiveLeafSystem->SetVectorParameter(FName("EmitterOrigin"),
				PawnLoc + FVector(0, 0, 400.f));
		}
	}
	else if (ActiveLeafSystem)
	{
		ActiveLeafSystem->Deactivate();
		ActiveLeafSystem = nullptr;
	}

	// --- Fireflies ---
	const bool bIsNight = Hour >= FireflyStartHour || Hour < FireflyEndHour;
	if (bEnableFireflies && bIsNight && !bRaining)
	{
		if (!ActiveFireflySystem)
		{
			UNiagaraSystem* FireflyNS = LoadObject<UNiagaraSystem>(nullptr,
				TEXT("/Game/Effects/NS_Fireflies.NS_Fireflies"));
			if (FireflyNS)
			{
				ActiveFireflySystem = UNiagaraFunctionLibrary::SpawnSystemAttached(
					FireflyNS, Pawn->GetRootComponent(), NAME_None,
					FVector::ZeroVector, FRotator::ZeroRotator,
					EAttachLocation::KeepRelativeOffset, true);
			}
		}

		if (ActiveFireflySystem)
		{
			ActiveFireflySystem->SetVariableFloat(FName("SpawnRate"), FireflySpawnRate);
			ActiveFireflySystem->SetVariableFloat(FName("GlowSpeed"), FireflyGlowSpeed);
			ActiveFireflySystem->SetVectorParameter(FName("EmitterOrigin"),
				PawnLoc + FVector(0, 0, 100.f));
		}
	}
	else if (ActiveFireflySystem)
	{
		ActiveFireflySystem->Deactivate();
		ActiveFireflySystem = nullptr;
	}

	// --- Rain Mist ---
	if (bEnableRainMist && bRaining)
	{
		if (!ActiveMistSystem)
		{
			UNiagaraSystem* MistNS = LoadObject<UNiagaraSystem>(nullptr,
				TEXT("/Game/Effects/NS_RainMist.NS_RainMist"));
			if (MistNS)
			{
				ActiveMistSystem = UNiagaraFunctionLibrary::SpawnSystemAttached(
					MistNS, Pawn->GetRootComponent(), NAME_None,
					FVector::ZeroVector, FRotator::ZeroRotator,
					EAttachLocation::KeepRelativeOffset, true);
			}
		}

		if (ActiveMistSystem)
		{
			ActiveMistSystem->SetVariableFloat(FName("SpawnRate"), MistSpawnRateRain);
			ActiveMistSystem->SetVectorParameter(FName("EmitterOrigin"),
				PawnLoc + FVector(0, 0, MistHeight));
		}
	}
	else if (ActiveMistSystem)
	{
		ActiveMistSystem->Deactivate();
		ActiveMistSystem = nullptr;
	}
}
