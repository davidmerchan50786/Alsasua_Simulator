// MegasionCoordinator.cpp
#include "Systemics/Megasion/MegasionCoordinator.h"
#include "Systemics/Megasion/MegaMarchSubsystem.h"
#include "Components/AudioComponent.h"
#include "Engine/World.h"
#include "Sound/AmbientSound.h"
#include "Sound/SoundBase.h"

namespace
{
	// Radios de los aros de audio alrededor del centro de la marcha (cm).
	constexpr float RadioMegafonos = 500.f;
	constexpr float RadioBanda = 1200.f;
}

void UMegasionCoordinator::StartMegasion(FVector Center, int32 Participants)
{
	if (bEventActive)
	{
		StopMegasion();
	}

	UWorld* Mundo = GetWorld();
	UMegaMarchSubsystem* Marcha = Mundo ? Mundo->GetSubsystem<UMegaMarchSubsystem>() : nullptr;
	if (!Mundo || !Mundo->HasBegunPlay() || !Marcha)
	{
		return;
	}

	Marcha->MaxProtesters = Participants;
	Marcha->StartMegaMarch(Center);

	EventCenter = Center;
	TotalParticipants = Participants;
	EventTimer = 0.f;
	bEventActive = true;

	SpawnAudioSources(Center);
}

void UMegasionCoordinator::StopMegasion()
{
	if (UMegaMarchSubsystem* Marcha = GetWorld() ? GetWorld()->GetSubsystem<UMegaMarchSubsystem>() : nullptr)
	{
		Marcha->StopMegaMarch();
	}

	for (const TObjectPtr<AAmbientSound>& Emisor : AudioSources)
	{
		if (IsValid(Emisor))
		{
			Emisor->Destroy();
		}
	}
	AudioSources.Empty();

	bEventActive = false;
	EventTimer = 0.f;
	TotalParticipants = 0;
}

FString UMegasionCoordinator::GetEventStatus() const
{
	return FString::Printf(TEXT("Megasion: %s | %.0f/%.0f s | %d participantes | %d fuentes de audio"),
		bEventActive ? TEXT("ACTIVO") : TEXT("INACTIVO"),
		EventTimer, EventDuration, TotalParticipants, AudioSources.Num());
}

void UMegasionCoordinator::Tick(float DeltaTime)
{
	EventTimer += DeltaTime;
	if (EventTimer >= EventDuration)
	{
		UE_LOG(LogTemp, Log, TEXT("Megasion: evento auto-detenido tras %.0f s"), EventTimer);
		StopMegasion();
	}
}

void UMegasionCoordinator::SpawnAudioSources(FVector Center)
{
	const float AnguloPaso = 360.f / FMath::Max(AudioSourcesPerType, 1);

	for (int32 i = 0; i < AudioSourcesPerType; ++i)
	{
		const float Angulo = FMath::DegreesToRadians(AnguloPaso * i);
		SpawnEmisor(MegaphoneSound,
			Center + FVector(FMath::Cos(Angulo) * RadioMegafonos, FMath::Sin(Angulo) * RadioMegafonos, 0.f));
		SpawnEmisor(BandSound,
			Center + FVector(FMath::Cos(Angulo) * RadioBanda, FMath::Sin(Angulo) * RadioBanda, 0.f));
	}
}

AAmbientSound* UMegasionCoordinator::SpawnEmisor(USoundBase* Sonido, const FVector& Posicion)
{
	UWorld* Mundo = GetWorld();
	if (!Mundo || !Sonido)
	{
		return nullptr; // sin asset asignado no hay emisor
	}

	FActorSpawnParameters Params;
	Params.SpawnCollisionHandlingOverride = ESpawnActorCollisionHandlingMethod::AlwaysSpawn;

	AAmbientSound* Emisor = Mundo->SpawnActor<AAmbientSound>(Posicion, FRotator::ZeroRotator, Params);
	if (!Emisor)
	{
		return nullptr;
	}

	if (UAudioComponent* Audio = Emisor->GetAudioComponent())
	{
		Audio->SetSound(Sonido);
		Audio->Play();
	}
	AudioSources.Add(Emisor);
	return Emisor;
}
