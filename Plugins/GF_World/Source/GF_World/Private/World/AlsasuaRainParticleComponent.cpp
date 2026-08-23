#include "World/AlsasuaRainParticleComponent.h"
#include "World/AlsasuaVisualEffectsManager.h"
#include "AlsasuaServiceRegistry.h"
#include "ContratosClima.h"
#include "NiagaraComponent.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Kismet/GameplayStatics.h"
#include "Camera/CameraComponent.h"

UAlsasuaRainParticleComponent::UAlsasuaRainParticleComponent()
{
	PrimaryComponentTick.bCanEverTick = true;
	PrimaryComponentTick.TickInterval = 0.1f;
}

void UAlsasuaRainParticleComponent::BeginPlay()
{
	Super::BeginPlay();

	if (!RainNiagaraAsset)
	{
		RainNiagaraAsset = LoadObject<UNiagaraSystem>(nullptr,
			TEXT("/Game/Effects/NS_Rain.NS_Rain"));
	}
	if (!SnowNiagaraAsset)
	{
		SnowNiagaraAsset = LoadObject<UNiagaraSystem>(nullptr,
			TEXT("/Game/Effects/NS_Snow.NS_Snow"));
	}
	if (!ThunderNiagaraAsset)
	{
		ThunderNiagaraAsset = LoadObject<UNiagaraSystem>(nullptr,
			TEXT("/Game/Effects/NS_ThunderFlash.NS_ThunderFlash"));
	}
}

void UAlsasuaRainParticleComponent::TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction)
{
	Super::TickComponent(DeltaTime, TickType, ThisTickFunction);
	UpdateRainState(DeltaTime);
}

void UAlsasuaRainParticleComponent::UpdateRainState(float DeltaTime)
{
	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	IWeatherService* Weather = [&]{ auto* R = UAlsasuaServiceRegistry::Get(W); return R ? R->PedirComo<IWeatherService>("Clima.Meteorologia") : nullptr; }();
	if (!VFXMgr || !Weather) return;

	const EAlsasuaWeatherState State = Weather->GetWeatherState();
	const float Wetness = VFXMgr->GlobalWetness;

	float TargetSpawnRate = 0.f;
	UNiagaraSystem* TargetSystem = nullptr;

	switch (State)
	{
	case EAlsasuaWeatherState::Rainy:
		TargetSpawnRate = FMath::Lerp(LightRainSpawnRate, HeavyRainSpawnRate, Wetness);
		TargetSystem = RainNiagaraAsset;
		bIsSnowing = false;
		break;

	case EAlsasuaWeatherState::Thunderstorm:
		TargetSpawnRate = HeavyRainSpawnRate;
		TargetSystem = RainNiagaraAsset;
		bIsSnowing = false;

		ThunderTimer += DeltaTime;
		if (ThunderTimer >= ThunderInterval)
		{
			ThunderTimer = 0.f;
			SpawnThunderFlash();
		}
		break;

	case EAlsasuaWeatherState::HeavyFog:
		TargetSpawnRate = SnowSpawnRate * 0.3f;
		TargetSystem = SnowNiagaraAsset;
		bIsSnowing = true;
		break;

	default:
		TargetSpawnRate = 0.f;
		break;
	}

	CurrentSpawnRate = FMath::FInterpTo(CurrentSpawnRate, TargetSpawnRate, DeltaTime, 3.f);

	if (CurrentSpawnRate < 1.f)
	{
		if (ActiveRainSystem)
		{
			ActiveRainSystem->Deactivate();
			ActiveRainSystem = nullptr;
		}
		return;
	}

	if (!ActiveRainSystem && TargetSystem)
	{
		AActor* Owner = GetOwner();
		if (!Owner) return;

		ActiveRainSystem = UNiagaraFunctionLibrary::SpawnSystemAttached(
			TargetSystem,
			Owner->GetRootComponent(),
			NAME_None,
			FVector::ZeroVector,
			FRotator::ZeroRotator,
			EAttachLocation::KeepRelativeOffset,
			true);

		if (ActiveRainSystem)
		{
			ActiveRainSystem->SetVariableFloat(FName("SpawnRate"), CurrentSpawnRate);
			ActiveRainSystem->SetVariableFloat(FName("WindStrength"), VFXMgr->WindIntensity * WindResponse);
		}
	}

	if (ActiveRainSystem)
	{
		ActiveRainSystem->SetVariableFloat(FName("SpawnRate"), CurrentSpawnRate);
		ActiveRainSystem->SetVariableFloat(FName("WindStrength"), VFXMgr->WindIntensity * WindResponse);
	}
}

void UAlsasuaRainParticleComponent::SpawnThunderFlash()
{
	UWorld* W = GetWorld();
	if (!W) return;

	APlayerController* PC = UGameplayStatics::GetPlayerController(W, 0);
	if (!PC || !PC->GetPawn()) return;

	if (ThunderNiagaraAsset)
	{
		UNiagaraFunctionLibrary::SpawnSystemAtLocation(
			W,
			ThunderNiagaraAsset,
			PC->GetPawn()->GetActorLocation() + FVector(0, 0, 5000.f));
	}

	UGameplayStatics::PlaySound2D(this, ThunderSound, 1.0f, FMath::RandRange(0.8f, 1.2f));

	ThunderFlashTimer = ThunderFlashDuration;

	APlayerCameraManager* CamMgr = PC->PlayerCameraManager;
	if (CamMgr)
	{
		CamMgr->StartCameraFade(1.f, 0.f, ThunderFlashDuration, FLinearColor(1.f, 1.f, 0.95f), false, true);
	}
}

void UAlsasuaRainParticleComponent::UpdateWindOnParticles()
{
	if (!ActiveRainSystem) return;

	UWorld* W = GetWorld();
	if (!W) return;

	UAlsasuaVisualEffectsManager* VFXMgr = W->GetSubsystem<UAlsasuaVisualEffectsManager>();
	if (!VFXMgr) return;

	ActiveRainSystem->SetVariableFloat(FName("WindStrength"),
		VFXMgr->WindIntensity * WindResponse);
	ActiveRainSystem->SetVariableFloat(FName("WindDirection"),
		VFXMgr->WindDirection);
}
