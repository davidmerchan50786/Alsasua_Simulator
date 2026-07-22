#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "OperativeCharacter.generated.h"

UCLASS()
class ALSASUAMANIFA_API AOperativeCharacter : public ACharacter {
    GENERATED_BODY()
public:
    AOperativeCharacter();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|DeepState")
    bool bIsDisguised = true;

    UFUNCTION(BlueprintCallable, Category="AAA|DeepState")
    void ExecuteAmbush();
};