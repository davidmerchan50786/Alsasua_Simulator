// DiaNocheSubsystem.h (capa GAMEPLAY)
// Reloj de juego + efectos de la hora. Puerto de SistemaDiaNoche.
// (Mientras no haya sistema de atmósfera, este subsistema ES la hora del juego;
//  la iluminación del mundo la leerá de aquí.)
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "AlsasuaTypes.h"
#include "DiaNocheSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UDiaNocheSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintReadOnly, Category="Tiempo") float Hora = 12.f;   // 0-24
	UPROPERTY(EditAnywhere, Category="Tiempo") float HorasPorSegundo = 0.01667f;   // ~24 min reales/día

	UFUNCTION(BlueprintPure, Category="Tiempo") bool EsNoche() const { return Hora >= 22.f || Hora < 6.f; }
	UFUNCTION(BlueprintPure, Category="Tiempo") bool EsDia() const { return !EsNoche(); }

	UFUNCTION(BlueprintPure, Category="Tiempo") float DeteccionSigilo() const { return EsNoche() ? 0.6f : 1.f; }
	UFUNCTION(BlueprintPure, Category="Tiempo") float FactorTrapicheo() const { return EsNoche() ? 1.5f : 0.8f; }
	UFUNCTION(BlueprintPure, Category="Tiempo") float FactorIngresoNegocio(ETipoNegocio Tipo) const;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UDiaNocheSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
};
