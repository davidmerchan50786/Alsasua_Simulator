#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Systems/Media/RadioBroadcastTypes.h"
#include "RadioSubsystem.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnRadioBroadcast, FRadioNewsClip, News);

UCLASS()
class GF_SOCIAL_API URadioSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    // Dispara una noticia basada en eventos del mundo
    UFUNCTION(BlueprintCallable, Category="AAA|Radio")
    void TriggerUrgentNews(FText Headline, FText Body, USoundBase* Voice = nullptr);

    // Sistema de "Canal de Telegram" (Notificaciones persistentes)
    UFUNCTION(BlueprintCallable, Category="AAA|Radio")
    void AddClandestineMessage(FText Sender, FText Message);

    UPROPERTY(BlueprintAssignable)
    FOnRadioBroadcast OnRadioUpdate;

    UPROPERTY(BlueprintAssignable)
    FOnRadioBroadcast OnTelegramUpdate;
};