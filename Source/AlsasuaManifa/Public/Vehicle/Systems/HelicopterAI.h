#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "HelicopterAI.generated.h"

UCLASS()
class ALSASUAMANIFA_API AHelicopterAI : public APawn {
    GENERATED_BODY()
public:
    AHelicopterAI();
    UPROPERTY(EditAnywhere, BlueprintReadWrite) AActor* Target;
    UPROPERTY(VisibleAnywhere) class USpotLightComponent* SearchLight;

    virtual void Tick(float DeltaTime) override;
};