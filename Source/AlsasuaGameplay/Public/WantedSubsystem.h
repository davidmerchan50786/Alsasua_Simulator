// WantedSubsystem.h (capa GAMEPLAY)
// Nivel de búsqueda (0-5) con desescalada temporal. Puerto de IWantedSystem /
// GameManager.AumentarBusqueda + SistemaEscapeWanted.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "WantedSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnEstrellasCambia, int32, Nivel);

UCLASS()
class ALSASUAGAMEPLAY_API UWantedSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Wanted") int32 NivelBusqueda = 0;   // 0-5
	UPROPERTY(EditAnywhere, Category="Wanted")      float TiempoBajarNivel = 20.f;
	UPROPERTY(BlueprintAssignable, Category="Wanted") FOnEstrellasCambia OnEstrellasCambia;

	// cantidad negativa baja el nivel (clamp 0-5), como en Unity.
	UFUNCTION(BlueprintCallable, Category="Wanted") void AumentarBusqueda(int32 Cantidad = 1);

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UWantedSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float TimerBajar = 0.f;
};
