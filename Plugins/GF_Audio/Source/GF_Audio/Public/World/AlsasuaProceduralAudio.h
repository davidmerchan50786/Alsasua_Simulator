#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaProceduralAudio.generated.h"

class UAudioComponent;
class USoundAttenuation;

/**
 * Sistema de audio procedural: viento, pájaros, agua.
 * Cambia ambientes sonoros según ubicación y clima.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_AUDIO_API UAlsasuaProceduralAudio : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaProceduralAudio();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Wind Ambience ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Wind")
	TObjectPtr<USoundBase> WindSoundCalm;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Wind")
	TObjectPtr<USoundBase> WindSoundStrong;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Wind")
	float WindVolumeMax = 0.4f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Wind")
	float WindFadeSpeed = 0.5f;

	// --- Bird Ambience ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Birds")
	TObjectPtr<USoundBase> BirdSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Birds")
	float BirdVolumeMax = 0.3f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Birds")
	float BirdStartHour = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Birds")
	float BirdEndHour = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Birds")
	float BirdDensityUrban = 0.2f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Birds")
	float BirdDensityNature = 0.8f;

	// --- Water Ambience ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Water")
	TObjectPtr<USoundBase> RiverSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Water")
	float RiverVolumeMax = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Water")
	float RiverNearDistance = 500.f;

	// --- Rain Ambience ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Rain")
	TObjectPtr<USoundBase> RainAmbience;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Rain")
	float RainVolumeMax = 0.6f;

	// --- Night Ambience ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Night")
	TObjectPtr<USoundBase> CricketSound;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Night")
	float CricketVolumeMax = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Night")
	float CricketStartHour = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Audio|Night")
	float CricketEndHour = 5.f;

private:
	void UpdateAudioLayers(float DeltaTime);

	UPROPERTY()
	TObjectPtr<UAudioComponent> WindAudio;

	UPROPERTY()
	TObjectPtr<UAudioComponent> BirdAudio;

	UPROPERTY()
	TObjectPtr<UAudioComponent> RiverAudio;

	UPROPERTY()
	TObjectPtr<UAudioComponent> RainAudio;

	UPROPERTY()
	TObjectPtr<UAudioComponent> CricketAudio;

	float CurrentWindVolume = 0.f;
	float CurrentBirdVolume = 0.f;
	float CurrentRiverVolume = 0.f;
	float CurrentRainVolume = 0.f;
	float CurrentCricketVolume = 0.f;
};
