#include "Vehicle/Systems/PoliceScannerComponent.h"

UPoliceScannerComponent::UPoliceScannerComponent()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UPoliceScannerComponent::InterceptPoliceComm(const FString& Message)
{
    if (bIsScannerActive)
    {
        OnRadioIntercepted.Broadcast(Message);
    }
}
