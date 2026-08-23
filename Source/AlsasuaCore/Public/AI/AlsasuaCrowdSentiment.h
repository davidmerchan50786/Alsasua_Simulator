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

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnConvocarManifestacion, FVector, Punto);

UCLASS()
class ALSASUACORE_API UAlsasuaCrowdSentiment : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void Tick(float DeltaTime) override;
	virtual bool IsAllowedToTick() const override { return true; }
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(AlsasuaCrowdSentiment, STATGROUP_Game); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

	UPROPERTY(BlueprintReadOnly, Category = "AAA|Social")
	float PopularSupport = 0.0f;

	// Inyecta un evento de tensión en una localización (ej: un arresto o carga)
	UFUNCTION(BlueprintCallable, Category = "AAA|Social")
	void TriggerSocialEvent(FVector Location, float Intensity, float Radius);

	// Obtiene el humor dominante en una zona de la ciudad
	ECrowdMood GetMoodAtLocation(FVector Location) const;

	// Delegate que dispara cuando alguien quiere convocar una manifestación.
	// ManifestacionSubsystem se suscribe; el Character lo dispara.
	UPROPERTY(BlueprintAssignable, Category = "AAA|Social")
	FOnConvocarManifestacion OnConvocarManifestacion;

	UPROPERTY(BlueprintReadOnly, Category = "AAA|Social")
	float GlobalTension = 0.0f;

private:
	TMap<FIntPoint, float> TensionMap;

	void DecayTension(float DeltaTime);
	void PropagateTension();
};
