// EconomiaSubsystem.h (capa GAMEPLAY)
// Dinero y puntuación del jugador. Puerto de IEconomyService / GameManager.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "EconomiaSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnEconomiaCambia, int32, Dinero, int32, Puntuacion);

UCLASS()
class ALSASUAGAMEPLAY_API UEconomiaSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Economia") int32 Dinero = 0;
	UPROPERTY(BlueprintReadOnly, Category="Economia") int32 Puntuacion = 0;
	UPROPERTY(BlueprintAssignable, Category="Economia") FOnEconomiaCambia OnEconomiaCambia;

	UFUNCTION(BlueprintCallable, Category="Economia") void GanarDinero(int32 Cantidad);
	UFUNCTION(BlueprintCallable, Category="Economia") bool GastarDinero(int32 Cantidad);
};
