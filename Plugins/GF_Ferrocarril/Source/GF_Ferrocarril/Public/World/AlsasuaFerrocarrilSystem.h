// AlsasuaFerrocarrilSystem.h (capa MANIFA)
// Aparca material rodante en la playa de vías de la estación de Altsasu.
//
// Altsasu es nudo ferroviario — ahí se cruzan la línea Madril-Hendaia y la de
// Castejón de Ebro —, y hasta ahora la estación era una cinta de balasto vacía.
// railways_unity.json trae los 86 trazados y los dos apeaderos; este sistema
// elige las vías de apartadero cercanas a la estación y les pone encima una
// locomotora con su rastra de contenedores.
//
// La locomotora y el contenedor llevaban tiempo bajados en AssetsImportados sin
// que ningún sistema los pidiera. Se resuelven por AlsasuaMallaFab, así que si
// no están, cae a la forma básica del motor y el mundo sigue arrancando.
//
// Las mallas descargadas vienen a la escala que le diera la gana a su autor, así
// que no se colocan tal cual: se miden sus bounds y se reescalan a la longitud
// real del vehículo (18 m la locomotora, 12,2 m el contenedor de 40 pies). Es la
// única forma de que un asset de origen desconocido case con una vía de ancho
// medido sobre el catastro.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "Arranque/AlsasuaPilarArranque.h"
#include "AlsasuaFerrocarrilSystem.generated.h"

UCLASS()
class GF_FERROCARRIL_API UAlsasuaFerrocarrilSystem : public UWorldSubsystem, public IAlsasuaPilarArranque
{
	GENERATED_BODY()

public:
	// IAlsasuaPilarArranque: fase 52 (material rodante) del antiguo Director.
	virtual int32 EjecutarArranque() override { return ColocarMaterialRodante(); }
	virtual FString EtiquetaArranque() const override { return TEXT("material rodante"); }
	virtual int32 OrdenArranque() const override { return 520; }

	/** Coloca las composiciones. Devuelve cuántos vehículos se han puesto. */
	UFUNCTION(BlueprintCallable, Category = "AAA|Ferrocarril")
	int32 ColocarMaterialRodante();

	/** Radio alrededor de la estación en el que se busca apartadero (m). */
	UPROPERTY(EditAnywhere, Category = "AAA|Ferrocarril")
	float RadioEstacionM = 450.f;

	/** Cuántas composiciones como mucho. Cada una son pocos draw calls, pero
	 *  una estación de pueblo no tiene una playa de vías infinita. */
	UPROPERTY(EditAnywhere, Category = "AAA|Ferrocarril")
	int32 MaxComposiciones = 3;

	/** Vagones por composición, sin contar la locomotora. */
	UPROPERTY(EditAnywhere, Category = "AAA|Ferrocarril")
	int32 VagonesPorComposicion = 6;

private:
	bool bHecho = false;
};
