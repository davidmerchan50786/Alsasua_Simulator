// GobernadorRender.h (capa WORLD)
// Director de GPU/CPU: vigila el frame-time suavizado y produce un radio de
// mundo dinámico. Bajo presión encoge el radio (menos draw calls); con holgura
// lo expande. Puerto de Core/Simulacion/GobernadorRender.cs (IRenderBudgetGovernor).
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "GobernadorRender.generated.h"

UCLASS()
class ALSASUAWORLD_API UGobernadorRender : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Presupuesto objetivo: 60 fps = 16.6 ms. Por encima => encoge.
	UPROPERTY(EditAnywhere, Category="Render") float MsObjetivo   = 16.6f;
	UPROPERTY(EditAnywhere, Category="Render") float RadioMin     = 15000.f;   // 150 m
	UPROPERTY(EditAnywhere, Category="Render") float RadioMax     = 200000.f;  // 2 km — colinas y horizonte reales de Alsasua
	UPROPERTY(EditAnywhere, Category="Render") float FactorImpostor = 2.0f;    // banda impostor = radio × esto

	float RadioActivacion() const { return Radio; }
	float RadioImpostor()   const { return Radio * FactorImpostor; }
	float FactorCarga()     const { return Carga; }   // 1=holgado, →0 saturado

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UGobernadorRender, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool DoesSupportWorldType(const EWorldType::Type Tipo) const override { return Tipo == EWorldType::Game || Tipo == EWorldType::PIE; }

private:
	float Radio   = 80000.f;   // 800 m de salida (horizonte realista del valle del Arakil)
	float MsSuave = 16.6f;
	float Carga   = 1.f;
};
