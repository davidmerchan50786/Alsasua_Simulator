// CalleGenerada.h (capa WORLD)
// Calle/carretera procedural: cinta plana de ancho dado drapeada sobre el
// terreno siguiendo una polilínea. Puerto de ConstructorCallesAssets / GeneradorMundoOSM (vías).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "CalleGenerada.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ALSASUAWORLD_API ACalleGenerada : public AActor
{
	GENERATED_BODY()

public:
	ACalleGenerada();

	// Puntos en mundo XY (cm). La Z se muestrea del terreno por vértice.
	void Construir(const TArray<FVector2D>& PuntosMundo, float AnchoCm);

	UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* Malla;
	UPROPERTY(EditAnywhere, Category="Calle") int32 Id = 0;
	UPROPERTY(EditAnywhere, Category="Calle") FString Tipo;
	UPROPERTY(EditAnywhere, Category="Calle") float EpsilonCm = 12.f;   // alzado sobre el terreno
	UPROPERTY(EditAnywhere, Category="Calle") FColor ColorBase = FColor(42, 42, 46, 255);  // asfalto por defecto

private:
	float AlturaSuelo(const FVector2D& XY) const;
};
