#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "NPCCharacter.generated.h"

UCLASS()
class ALSASUAENTITIES_API ANPCCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    ANPCCharacter();
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Alsasua|NPC")
    float Morale = 50.0f;
};
