// HerrikoPlazaGenerator.h (capa WORLD)
// Genera la geometría específica de Herriko Plaza: fuente central,
// pavimento de adoquines, bancos, y el Nogal monument.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "HerrikoPlazaGenerator.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ALSASUAWORLD_API AHerrikoPlazaGenerator : public AActor
{
	GENERATED_BODY()

public:
	AHerrikoPlazaGenerator();

	virtual void BeginPlay() override;

	void Generar();

private:
	void GenerarFuente(UProceduralMeshComponent* Malla);
	void GenerarPavimento(UProceduralMeshComponent* Malla);
	void GenerarBancos(UProceduralMeshComponent* Malla);

	UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* MallaPlaza;
};
