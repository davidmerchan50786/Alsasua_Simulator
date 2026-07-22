// GuardadoSubsystem.h (capa GAMEPLAY)
// Guarda/carga la partida agrupando el estado de economía, apoyo, búsqueda,
// hora, misión y jugador. Puerto de SistemaGuardado (GuardarEnSlot/Cargar).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "GuardadoSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UGuardadoSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UFUNCTION(BlueprintCallable, Category="Guardado") bool GuardarEnSlot(int32 Slot = 0);
	UFUNCTION(BlueprintCallable, Category="Guardado") bool CargarDeSlot(int32 Slot = 0);
	UFUNCTION(BlueprintCallable, Category="Guardado") bool ExisteGuardado(int32 Slot = 0) const;

	// Lo pone "Continuar" del menú principal: el juego carga el slot al arrancar.
	UPROPERTY() bool bCargarAlArrancar = false;

private:
	static FString NombreSlot(int32 Slot) { return FString::Printf(TEXT("AlsasuaSlot%d"), Slot); }
};
