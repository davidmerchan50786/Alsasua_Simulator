#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AlsasuaCrowdAgentInterface.generated.h"

/** Contrato del tronco hacia los agentes de multitud (GF_NPCs): el megafono
 *  y otros sistemas de influencia ajustan su moral sin conocer el pilar. */
UINTERFACE(MinimalAPI)
class UAlsasuaCrowdAgentInterface : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaCrowdAgentInterface
{
	GENERATED_BODY()

public:
	/** Suma (o resta, con negativo) moral al agente de multitud. */
	UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Alsasua|Multitud")
	void AdjustMorale(float Delta);
};
