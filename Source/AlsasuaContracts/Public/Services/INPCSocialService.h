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

    /**
     * Moviliza peatones propensos a protestar (Rebelde/Sociable/Amable) que
     * estén a Radio del Punto: los mueve al punto, los marca manifestante y
     * les da voz. Devuelve cuántos se movilizaron (0..MaxN). Los escenarios
     * que necesiten N ejecutores espawnean clones solo por la diferencia.
     */
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Contracts|NPCSocial")
    virtual int32 ReclutarPropensos(const FVector& Punto, float Radio, int32 MaxN) = 0;
};
