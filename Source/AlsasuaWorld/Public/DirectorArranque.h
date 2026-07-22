#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "DirectorArranque.generated.h"

UCLASS()
class ALSASUAWORLD_API ADirectorArranque : public AActor
{
    GENERATED_BODY()
public:
    virtual void BeginPlay() override;
    UFUNCTION(BlueprintCallable, Category = "Alsasua|World")
    void IniciarConstruccion();
};
