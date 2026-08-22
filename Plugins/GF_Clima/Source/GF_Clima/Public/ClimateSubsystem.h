#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "ClimateSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnClimateWeatherChanged, bool, bIsRaining);

UCLASS()
class GF_CLIMA_API UClimateSubsystem : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	// Hora del día (0.0 a 24.0)
	UPROPERTY(BlueprintReadOnly, Category = "Climate")
	float CurrentTime;

	// Flag de lluvia (afecta visibilidad e IA)
	UPROPERTY(BlueprintReadOnly, Category = "Climate")
	bool bIsRaining;

	// Modificador de visibilidad para la IA (1.0 es normal, 0.5 es noche/lluvia)
	UFUNCTION(BlueprintPure, Category = "Climate")
	float GetVisibilityMultiplier() const;

	UPROPERTY(BlueprintAssignable, Category = "Climate")
	FOnClimateWeatherChanged OnWeatherChanged;

	void UpdateClimate(float DeltaTime);
};
