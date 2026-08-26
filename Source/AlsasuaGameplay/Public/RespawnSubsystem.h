// RespawnSubsystem.h (capa GAMEPLAY)
// Guarda el último punto de reaparición (piso franco visitado). Puerto del
// flujo de respawn de GameManager / SistemaGuardado.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "RespawnSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API URespawnSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;

	UPROPERTY(BlueprintReadOnly, Category="Respawn") bool bTienePunto = false;
	UPROPERTY(BlueprintReadOnly, Category="Respawn") FVector Punto = FVector::ZeroVector;

	UFUNCTION(BlueprintCallable, Category="Respawn")
	void FijarPunto(FVector P) { Punto = P; bTienePunto = true; }

	UFUNCTION(BlueprintCallable, Category="Respawn")
	bool Reaparecer(class APawn* Jugador) const;

private:
	UFUNCTION()
	void HandlePlayerDeath();
};
