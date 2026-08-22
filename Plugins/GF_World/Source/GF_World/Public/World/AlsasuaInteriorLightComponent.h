#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaInteriorLightComponent.generated.h"

class UPointLightComponent;

/**
 * Componente de luz interior para edificios. Genera luces puntuales
 * dentro de ventanas visibles desde el exterior, creando la ilusión
 * de habitaciones iluminadas de noche.
 */
UCLASS(ClassGroup = (Custom), meta = (BlueprintSpawnableComponent))
class GF_WORLD_API UAlsasuaInteriorLightComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UAlsasuaInteriorLightComponent();

	virtual void BeginPlay() override;
	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

	// --- Config ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	int32 NumLightsPerFloor = 3;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	int32 MaxFloors = 4;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	float FloorHeight = 300.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	float LightIntensity = 400.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	float LightRadius = 200.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	FLinearColor WarmLightColor = FLinearColor(1.f, 0.85f, 0.6f);

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Light")
	FLinearColor CoolLightColor = FLinearColor(0.7f, 0.8f, 1.f);

	// --- Behavior ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Behavior")
	float OnProbability = 0.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Behavior")
	float TurnOnHour = 17.5f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Behavior")
	float TurnOffHour = 7.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Behavior")
	float WarmCoolBlend = 0.5f;

	// --- Activity (walking lights) ---
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Activity")
	bool bEnableActivity = true;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Activity")
	float ActivitySpeed = 50.f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Interior|Activity")
	float ActivityRadius = 80.f;

private:
	void SetupInteriorLights();
	void UpdateInteriorLights(float DeltaTime);

	UPROPERTY()
	TArray<TObjectPtr<UPointLightComponent>> InteriorLights;

	UPROPERTY()
	TArray<bool> LightActive;

	UPROPERTY()
	TArray<float> LightFlickerPhase;

	float CurrentBlend = 0.f;
	bool bInitialized = false;
};
