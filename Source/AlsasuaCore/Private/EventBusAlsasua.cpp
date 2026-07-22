#include "EventBusAlsasua.h"

void UEventBusAlsasua::BroadcastEvent(FName EventName, UObject* Payload)
{
    UE_LOG(LogTemp, Log, TEXT("EventBus: %s"), *EventName.ToString());
}
