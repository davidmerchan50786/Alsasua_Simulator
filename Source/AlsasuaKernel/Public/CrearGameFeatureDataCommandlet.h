#pragma once

#include "Commandlets/Commandlet.h"
#include "CrearGameFeatureDataCommandlet.generated.h"

/**
 * Crea el asset GameFeatureData.uasset que exige cada plugin GF_* con
 * contenido: el motor busca /<Plugin>/GameFeatureData.GameFeatureData al
 * registrar un game feature plugin; sin el, cae en Plugin_Missing_GameFeatureData.
 *
 * Uso (una sola vez):
 *   UnrealEditor-Cmd.exe <proyecto> -run=CrearGameFeatureDataCommandlet -unattended
 */
UCLASS()
class UCrearGameFeatureDataCommandlet : public UCommandlet
{
	GENERATED_BODY()

public:
	UCrearGameFeatureDataCommandlet();

	virtual int32 Main(const FString& Params) override;
};
