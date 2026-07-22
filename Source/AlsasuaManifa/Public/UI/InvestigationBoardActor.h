#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "UI/InvestigationBoardWidget.h"
#include "InvestigationBoardActor.generated.h"

UCLASS()
class ALSASUAMANIFA_API AInvestigationBoardActor : public AActor
{
    GENERATED_BODY()

public:
    AInvestigationBoardActor();

    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="AAA|Investigacion")
    TArray<FNodoInvestigacion> TodosLosNodos;

    UFUNCTION(BlueprintCallable, Category="AAA|Investigacion")
    void DescubrirNodo(FName NodeId);

    UFUNCTION(BlueprintCallable, Category="AAA|Investigacion")
    void MarcarNodoComoSaboteado(FName NodeId);

    UFUNCTION(BlueprintCallable, Category="AAA|Investigacion")
    void LanzarMisionDeSabotaje(FName NodeId);

protected:
    virtual void BeginPlay() override;
};
