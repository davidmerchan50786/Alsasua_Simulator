#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "INPCSocialService.generated.h"

UINTERFACE(MinimalAPI, BlueprintType, NotBlueprintable)
class UNPCSocialService : public UInterface
{
    GENERATED_BODY()
};

class ALSASUACONTRACTS_API INPCSocialService
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|NPCSocial")
    virtual int32 GetNearestNPC(const FVector& Location, float MaxRadius) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|NPCSocial")
    virtual FString GetPersonaNombre(int32 Index) const = 0;

    /** Clave de fichero de diálogo por personalidad (Amable, Rebelde...). */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|NPCSocial")
    virtual FString GetPersonaClave(int32 Index) const = 0;

    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|NPCSocial")
    virtual FString HablarConNPC(int32 Index) = 0;
};
