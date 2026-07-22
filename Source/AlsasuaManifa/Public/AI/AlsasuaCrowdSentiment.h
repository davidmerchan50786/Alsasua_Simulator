#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "AlsasuaCrowdSentiment.generated.h"

/** Estados de ánimo de la manifestación */
UENUM(BlueprintType)
enum class ECrowdMood : uint8
{
	Calm,       // Manifestación pacífica, cánticos bajos
	Agitated,   // Tensión subiendo, movimientos más rápidos
	Hostile,    // Enfrentamiento inminente, empujones
	Panic       // Dispersión desordenada
};

UCLASS()
class ALSASUAMANIFA_API UAlsasuaCrowdSentiment : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsAllowedToTick() const { return true; }
	virtual TStatId GetStatId() const { return TStatId(); }

	UPROPERTY(BlueprintReadOnly, Category = "AAA|Social")
	float PopularSupport = 0.0f;

	// Inyecta un evento de tensión en una localización (ej: un arresto o carga)
	UFUNCTION(BlueprintCallable, Category = "AAA|Social")
	void TriggerSocialEvent(FVector Location, float Intensity, float Radius);

	// Obtiene el humor dominante en una zona de la ciudad
	ECrowdMood GetMoodAtLocation(FVector Location) const;

	UPROPERTY(BlueprintReadOnly, Category = "AAA|Social")
	float GlobalTension = 0.0f;

private:
	// Mapa de tensión por zonas (integrado con el SpatialGrid en el futuro)
	TMap<FIntPoint, float> TensionMap;

	void DecayTension(float DeltaTime);
	void PropagateTension();
};
