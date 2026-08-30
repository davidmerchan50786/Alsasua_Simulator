// LluviaDeHecesSubsystem.h (capa GAMEPLAY)
// Evento atmosférico absurdo: cae materia fecal sobre el jugador mientras la
// vista se tiñe de sepia. Se activa con la consola:
//   Alsasua.LluviaDeHeces        0|1   toggle
//   Alsasua.LluviaDeHecesIntervalo     segundos entre oleadas
//   Alsasua.LluviaDeHecesRadio         radio sobre el jugador
//   Alsasua.LluviaDeHecesCantidad      caídas por oleada
//   Alsasua.LluviaDeHecesAltura        altura de lanzamiento
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Tickable.h"
#include "LluviaDeHecesSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API ULluviaDeHecesSubsystem : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	virtual void OnWorldBeginPlay(UWorld& InWorld) override;
	virtual void Deinitialize() override;

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(ULluviaDeHecesSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	float Acum = 0.f;
	bool bLastActiva = false;
};

// Console-tunable state (file-static, survives PIE).
namespace LluviaDeHecesConsole
{
	extern int32 bActiva;
	extern float Intervalo;
	extern float Radio;
	extern int32 Cantidad;
	extern float Altura;
}