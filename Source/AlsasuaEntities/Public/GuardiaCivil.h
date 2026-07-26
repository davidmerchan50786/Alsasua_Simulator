#pragma once
#include "CoreMinimal.h"
#include "NPCCharacter.h"
#include "GuardiaCivil.generated.h"

UCLASS()
class ALSASUAENTITIES_API AGuardiaCivil : public ANPCCharacter
{
    GENERATED_BODY()
public:
    AGuardiaCivil();

    /** Radio de patrulla alrededor del punto inicial (cm). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|Patrol")
    float PatrolRadius = 3000.0f;

protected:
    virtual void BeginPlay() override;
};
