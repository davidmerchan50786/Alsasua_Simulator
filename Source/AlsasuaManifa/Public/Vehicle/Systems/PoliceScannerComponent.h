#pragma once
#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "PoliceScannerComponent.generated.h"

UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class ALSASUAMANIFA_API UPoliceScannerComponent : public UActorComponent {
    GENERATED_BODY()
public:
    UPoliceScannerComponent();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Scanner")
    bool bIsScannerActive = true;

    UFUNCTION(BlueprintCallable, Category="AAA|Scanner")
    void InterceptPoliceComm(const FString& Message);

    // Evento para UI: Muestra el texto de la radio policial
    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadioIntercepted, const FString&, Message);
    UPROPERTY(BlueprintAssignable)
    FOnRadioIntercepted OnRadioIntercepted;
};