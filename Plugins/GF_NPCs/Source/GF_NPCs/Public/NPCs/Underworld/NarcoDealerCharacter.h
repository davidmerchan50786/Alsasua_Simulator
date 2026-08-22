#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NarcoDealerCharacter.generated.h"

UCLASS()
class GF_NPCS_API ANarcoDealerCharacter : public ACharacter {
    GENERATED_BODY()
public:
    ANarcoDealerCharacter();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Underworld")
    bool bIsProtectedByDeepState = true;

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Underworld")
    float ImmunityLevel = 100.f; // Nivel de "inmunidad" legal actual

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Underworld")
    FName HandlerName = "Comisario X";

    // Función para ofrecer inmunidad a cambio de información (Traición)
    UFUNCTION(BlueprintCallable, Category="AAA|Underworld")
    void NegotiateImmunity(bool bAccept);

    // Función para exponer al camello ante la opinión pública
    UFUNCTION(BlueprintCallable, Category="AAA|Underworld")
    void ExposeDealer();

protected:
    virtual void BeginPlay() override;
};