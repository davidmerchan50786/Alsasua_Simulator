#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "DialogTextFormatter.generated.h"

UCLASS()
class GF_DIALOGOS_API UDialogTextFormatter : public UBlueprintFunctionLibrary {
    GENERATED_BODY()
public:
    // Transforma "Hola {PlayerName}" en "Hola Mikel"
    UFUNCTION(BlueprintPure, Category="AAA|Dialog")
    static FText FormatDialogueText(FText OriginalText, AActor* ContextActor);
};
