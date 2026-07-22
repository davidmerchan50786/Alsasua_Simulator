#pragma once
#include "CoreMinimal.h"
#include "Blueprint/UserWidget.h"
#include "Systems/Dialog/DialogTypes.h"
#include "DialogWidgetBase.generated.h"

UCLASS(Abstract)
class ALSASUAMANIFA_API UDialogWidgetBase : public UUserWidget {
    GENERATED_BODY()
public:
    // Evento para que el Blueprint dibuje el texto del NPC
    UFUNCTION(BlueprintImplementableEvent, Category="AAA|Dialog")
    void OnUpdateNPCText(const FText& Text, USoundBase* VoiceOver);

    // Evento para que el Blueprint limpie y dibuje los botones de opciones
    UFUNCTION(BlueprintImplementableEvent, Category="AAA|Dialog")
    void OnUpdateOptions(const TArray<FDialogOption>& Options);

    // Recibe el nodo actual del diálogo y actualiza la UI
    UFUNCTION(BlueprintCallable, Category="AAA|Dialog")
    void Internal_OnNodeReached(FDialogNode Node);

    void BindToInstance(class UDialogInstance* Instance);

protected:
    UPROPERTY(BlueprintReadOnly)
    class UDialogInstance* CurrentInstance;
};