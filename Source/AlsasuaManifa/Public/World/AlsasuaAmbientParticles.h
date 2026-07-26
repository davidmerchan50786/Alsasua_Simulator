#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaAmbientParticles.generated.h"

class UNiagaraComponent;

/**
 * Sistema de partículas ambientales: polvo, polen, hojas, luciérnagas.
 * Crea efectos atmosféricos que dan vida al mundo.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaAmbientParticles : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaAmbientParticles();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Dust ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Dust")
	bool bEnableDust = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Dust")
	float DustSpawnRate = 15.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Dust")
	float DustWindSensitivity = 0.8f;

	// --- Pollen ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Pollen")
	bool bEnablePollen = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Pollen")
	float PollenSpawnRate = 8.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Pollen")
	float PollenSeasonStart = 0.25f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Pollen")
	float PollenSeasonEnd = 0.75f;

	// --- Falling Leaves ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Leaves")
	bool bEnableLeaves = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Leaves")
	float LeafSpawnRate = 5.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Leaves")
	float AutumnStart = 0.7f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Leaves")
	float AutumnEnd = 0.9f;

	// --- Fireflies ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Fireflies")
	bool bEnableFireflies = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Fireflies")
	float FireflySpawnRate = 12.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Fireflies")
	float FireflyStartHour = 20.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Fireflies")
	float FireflyEndHour = 4.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Fireflies")
	float FireflyGlowSpeed = 2.f;

	// --- Rain Mist ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Mist")
	bool bEnableRainMist = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Mist")
	float MistSpawnRateRain = 25.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Ambient|Mist")
	float MistHeight = 150.f;

private:
	void UpdateAmbientParticles(float DeltaTime);

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveDustSystem;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActivePollenSystem;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveLeafSystem;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveFireflySystem;

	UPROPERTY()
	TObjectPtr<UNiagaraComponent> ActiveMistSystem;

	float TimeAccum = 0.f;
};
