#include "EventBusAlsasua.h"

void UEventBusAlsasua::BroadcastEvent(FName EventName, UObject* Payload)
{
    UE_LOG(LogTemp, Log, TEXT("EventBus: %s"), *EventName.ToString());
    OnEvent.Broadcast(EventName, Payload);

    if (TArray<TWeakObjectPtr<UObject>>* ListenerList = Listeners.Find(EventName))
    {
        for (int32 i = ListenerList->Num() - 1; i >= 0; --i)
        {
            if ((*ListenerList)[i].IsValid())
            {
                UFunction* Func = (*ListenerList)[i]->FindFunction(FName(TEXT("OnAlsasuaEvent")));
                if (Func && Func->NumParms == 2)
                {
                    FFrame Frame(Func, (*ListenerList)[i].Get());
                    FName Param0 = EventName;
                    UObject* Param1 = Payload;
                    Frame.ProcessCurrentScript(Func, &Param0, &Param1);
                }
            }
            else
            {
                ListenerList->RemoveAt(i);
            }
        }
    }
}

void UEventBusAlsasua::BroadcastEventNative(FName EventName, UObject* Payload)
{
    BroadcastEvent(EventName, Payload);
}

void UEventBusAlsasua::ListenEvent(FName EventName, UObject* Listener)
{
    if (!Listener) return;

    TArray<TWeakObjectPtr<UObject>>& ListenerList = Listeners.FindOrAdd(EventName);
    for (const auto& L : ListenerList)
    {
        if (L.Get() == Listener) return;
    }
    ListenerList.Add(Listener);
}
