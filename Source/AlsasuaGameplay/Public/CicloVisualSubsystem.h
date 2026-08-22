// CicloVisualSubsystem.h (capa GAMEPLAY)
// Cielo dinámico: orienta el sol y ajusta luz/niebla según la hora del
// DiaNocheSubsystem. Crea sol + skylight + niebla + atmósfera si no existen.
// Puerto de la parte de ciclo día/noche de SistemaVolumenHDRP.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "CicloVisualSubsystem.generated.h"

class ADirectionalLight;
class ASkyLight;
class AExponentialHeightFog;

UCLASS()
class ALSASUAGAMEPLAY_API UCicloVisualSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Cielo") float IntensidadDia   = 8.f;    // lux relativos del sol a mediodía
	UPROPERTY(EditAnywhere, Category="Cielo") float IntensidadNoche = 0.3f;   // luz de luna

	// UAlsasuaAtmosphereController es el único dueño activo de sol, cielo y niebla.
	virtual bool ShouldCreateSubsystem(UObject* Outer) const override { return false; }
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UCicloVisualSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	bool bCreado = false;
	float TiempoActualizacion = 0.f;
	UPROPERTY() ADirectionalLight* Sol = nullptr;
	UPROPERTY() ASkyLight* CieloLuz = nullptr;
	UPROPERTY() AExponentialHeightFog* Niebla = nullptr;
	UPROPERTY() AActor* Atmosfera = nullptr;

	void CrearCielo(class UWorld* W);
	void Actualizar(float Hora);
};
