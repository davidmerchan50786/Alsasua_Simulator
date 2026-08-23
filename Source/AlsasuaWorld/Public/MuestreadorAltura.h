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

	/**
	 * ¿Hay heightmap al que preguntar?
	 *
	 * Busca el terreno si aún no lo tiene, igual que hace AlturaMundo(). Antes
	 * sólo miraba el puntero, y ese puntero lo rellena BuscarTerreno() desde
	 * dentro de AlturaMundo(): o sea que el primero en preguntar recibía
	 * siempre false, aunque el terreno llevara rato construido. Quien lo
	 * consulta como guarda —`EstaDisponible() && DentroDelTerreno(...)`— se
	 * quedaba con el cortocircuito y descartaba TODO: los 2783 árboles del
	 * LiDAR se perdían y el log los contaba como "fuera del terreno", que es
	 * cierto de 280 de ellos y de ninguno de los otros 2503.
	 */
	bool EstaDisponible() const
	{
		if (!Terreno) const_cast<UMuestreadorAltura*>(this)->BuscarTerreno();
		return Terreno != nullptr;
	}

	bool HayAgua() const { return bHayAgua; }

	void RegistrarAlturaAgua(float Altura) { AlturaNivelAgua = Altura; bHayAgua = true; }

private:
	UPROPERTY() ATerrenoGenerado* Terreno = nullptr;
	float AlturaNivelAgua = -1393.9f;
	bool bHayAgua = false;

	void BuscarTerreno();
};
