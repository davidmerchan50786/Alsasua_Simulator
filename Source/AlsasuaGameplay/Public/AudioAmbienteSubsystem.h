// AudioAmbienteSubsystem.h (capa GAMEPLAY)
// Director de ambiente sonoro: mezcla camas de audio en bucle (lluvia, viento,
// multitud, día/noche) según el clima, la manifestación y la hora. Carga los
// sonidos por ruta blanda; si faltan, esa cama queda en silencio. Puerto de la
// parte ambiental de AudioManager/SistemaReverbZonas.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "AudioAmbienteSubsystem.generated.h"

class UAudioComponent;

UCLASS()
class ALSASUAGAMEPLAY_API UAudioAmbienteSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Audio") float SuavizadoPorSeg = 1.5f;   // velocidad de fundido
	UPROPERTY(BlueprintReadWrite, Category="Audio") float VolumenMaestro = 1.f;   // 0..1 (opciones)
	UPROPERTY(EditAnywhere, Category="Audio") float DuckingDialogo = 0.35f;       // ambiente baja con diálogo

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UAudioAmbienteSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	bool bInit = false;
	UPROPERTY() UAudioComponent* Lluvia = nullptr;
	UPROPERTY() UAudioComponent* Viento = nullptr;
	UPROPERTY() UAudioComponent* Multitud = nullptr;
	UPROPERTY() UAudioComponent* AmbDia = nullptr;
	UPROPERTY() UAudioComponent* AmbNoche = nullptr;

	UAudioComponent* CrearCama(const TCHAR* Ruta);
	void FundirA(UAudioComponent* C, float Objetivo, float DeltaTime);
};
