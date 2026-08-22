#pragma once

#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "InvestigationBoardWidget.generated.h"

USTRUCT(BlueprintType)
struct FNodoInvestigacion {
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FName NodeId;
    UPROPERTY(BlueprintReadOnly) FString Nombre;
    UPROPERTY(BlueprintReadOnly) FString Descripcion;
    UPROPERTY(BlueprintReadOnly) bool bDescubierto = false;
    UPROPERTY(BlueprintReadOnly) bool bSaboteado = false;
};

UCLASS()
class GF_UI_API UInvestigationBoardWidget : public UUserWidget
{
    GENERATED_BODY()

public:
    UFUNCTION(BlueprintImplementableEvent, Category="AAA|Investigacion")
    void RefreshBoard(const TArray<FNodoInvestigacion>& Nodos);

    UFUNCTION(BlueprintCallable, Category="AAA|Investigacion")
    void RequestStartMission(FName NodeId);
};
