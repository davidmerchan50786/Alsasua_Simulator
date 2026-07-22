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
	UPROPERTY(BlueprintReadOnly, Category="Respawn") bool bTienePunto = false;
	UPROPERTY(BlueprintReadOnly, Category="Respawn") FVector Punto = FVector::ZeroVector;

	UFUNCTION(BlueprintCallable, Category="Respawn")
	void FijarPunto(FVector P) { Punto = P; bTienePunto = true; }

	// Reaparece el pawn del jugador en el último punto (si hay).
	UFUNCTION(BlueprintCallable, Category="Respawn")
	bool Reaparecer(class APawn* Jugador) const;
};
