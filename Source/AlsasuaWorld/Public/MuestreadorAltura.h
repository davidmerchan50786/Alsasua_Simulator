#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "MuestreadorAltura.generated.h"

class ATerrenoGenerado;

UCLASS()
class ALSASUAWORLD_API UMuestreadorAltura : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;

	float AlturaMundo(const FVector& Pos) const;
	FVector NormalMundo(const FVector& Pos) const;
	bool EstaDisponible() const { return Terreno != nullptr; }

private:
	UPROPERTY() ATerrenoGenerado* Terreno = nullptr;
	void BuscarTerreno();
};
