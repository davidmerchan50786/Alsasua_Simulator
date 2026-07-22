#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlsasuaEntitiesCharacter.generated.h"

UCLASS()
class ALSASUAENTITIES_API AAlsasuaEntitiesCharacter : public ACharacter
{
    GENERATED_BODY()
public:
    AAlsasuaEntitiesCharacter();
    virtual void SetupPlayerInputComponent(class UInputComponent* PlayerInputComponent) override;

    UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "Alsasua")
    class UCameraComponent* CameraComponent;
};
