// IInteractuable.h - DEPRECATED: Usar IInteractableInterface.h
#pragma once
#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "IInteractuable.generated.h"

UINTERFACE(MinimalAPI, BlueprintType)
class UInteractuable : public UInterface
{
    GENERATED_BODY()
};

class GF_AI_API IInteractuable
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintNativeEvent, BlueprintCallable, Category = "Interaccion")
    void Interactuar(UObject* Jugador);
};
