#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "DialogComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_DIALOGOS_API UDialogComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Dialog")
    class UDialogAsset* MainDialog;

    UFUNCTION(BlueprintCallable, Category="AAA|Dialog")
    void Interact(AActor* Interactor);
};
