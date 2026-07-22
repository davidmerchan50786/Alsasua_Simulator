// ProgresionSubsystem.h (capa GAMEPLAY)
// Nivel del movimiento (0-5) derivado del apoyo popular, con ventajas.
// Puerto de SistemaProgresion.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "ProgresionSubsystem.generated.h"

UCLASS()
class ALSASUAGAMEPLAY_API UProgresionSubsystem : public UGameInstanceSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	UPROPERTY(BlueprintReadOnly, Category="Progresion") int32 Nivel = 0;

	UFUNCTION(BlueprintPure, Category="Progresion") float MultiplicadorIngresos() const { return 1.f + Nivel * 0.15f; }
	UFUNCTION(BlueprintPure, Category="Progresion") float DescuentoTienda() const { return FMath::Min(0.30f, Nivel * 0.06f); }
	UFUNCTION(BlueprintPure, Category="Progresion") int32 ReduccionCalor() const { return Nivel >= 3 ? 1 : 0; }

private:
	UFUNCTION() void OnApoyo(float Apoyo);
};
