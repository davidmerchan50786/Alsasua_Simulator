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
	FLinearColor DatosSuelo(const FVector& Pos) const;
	float AlturaAgua() const { return AlturaNivelAgua; }
	bool EstaDisponible() const { return Terreno != nullptr; }
	bool HayAgua() const { return bHayAgua; }

	void RegistrarAlturaAgua(float Altura) { AlturaNivelAgua = Altura; bHayAgua = true; }

private:
	UPROPERTY() ATerrenoGenerado* Terreno = nullptr;
	float AlturaNivelAgua = -1393.9f;
	bool bHayAgua = false;

	void BuscarTerreno();
};
