// PoligonoSuelo.h (capa WORLD)
// Superficie plana (plaza, zona verde) triangulada y drapeada sobre el terreno.
// Puerto de los generadores OSM de plazas/greenspaces.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PoligonoSuelo.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ALSASUAWORLD_API APoligonoSuelo : public AActor
{
	GENERATED_BODY()

public:
	APoligonoSuelo();

	// Anillo en mundo XY (cm). Z muestreada del terreno por vértice.
	void Construir(const TArray<FVector2D>& AnilloMundo, FColor Color, float EpsilonCm = 6.f);

	UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* Malla;
	UPROPERTY(EditAnywhere, Category="Suelo") int32 Id = 0;
	UPROPERTY(EditAnywhere, Category="Suelo") FString Tipo;

private:
	float AlturaSuelo(const FVector2D& XY) const;
};
