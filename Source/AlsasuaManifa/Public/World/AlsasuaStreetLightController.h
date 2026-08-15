#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaStreetLightController.generated.h"

class UPointLightComponent;
class USpotLightComponent;

/**
 * Controlador de farolas/alumbrado público. Se añade a cada farola
 * y enciende/apaga automáticamente según la hora del día.
 * Soporta parpadeo, intensidad variable, y color por barrio.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UAlsasuaStreetLightController : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaStreetLightController();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Light Components ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Refs")
	TObjectPtr<UPointLightComponent> PointLight;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Refs")
	TObjectPtr<USpotLightComponent> SpotLight;

	// --- Timing ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Timing")
	float TurnOnHour = 18.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Timing")
	float TurnOffHour = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Timing")
	float FadeInTime = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Timing")
	float FadeOutTime = 0.5f;

	// --- Light Properties ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Light")
	float MaxIntensity = 8000.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Light")
	FLinearColor WarmColor = FLinearColor(1.0f, 0.85f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Light")
	float LightRadius = 1200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Light")
	float LightHeight = 400.f;

	// --- Flicker ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Flicker")
	bool bEnableFlicker = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Flicker")
	float FlickerChance = 0.02f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Flicker")
	float FlickerDuration = 0.08f;

	/** Fracción de farolas del pueblo que pueden parpadear (se sortea al empezar). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Flicker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float FlickerFixtureChance = 0.08f;

	/** Probabilidad de que esta farola nazca fundida (se sortea una sola vez). */
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "StreetLight|Flicker", meta = (ClampMin = "0.0", ClampMax = "1.0"))
	float BurnOutChance = 0.02f;

private:
	void UpdateLightState(float DeltaTime);
	void ApplyFlicker(float DeltaTime);
	void SetupLightComponents();

	float CurrentIntensity = 0.f;
	float TargetIntensity = 0.f;
	float FlickerTimer = 0.f;
	float FlickerScale = 1.f;
	bool bIsOn = false;
	bool bBurnedOut = false;
	bool bFlickers = false;
};
