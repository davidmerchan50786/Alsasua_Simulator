#include "Systems/Dialog/DialogTextFormatter.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

FText UDialogTextFormatter::FormatDialogueText(FText OriginalText, AActor* ContextActor) {
    FString BaseString = OriginalText.ToString();

    // Sustitución Simple de Parámetros (Extensible)
    if (BaseString.Contains("{PlayerName}")) {
        FString PName = "Mikel"; // Default del prota
        if (APawn* P = UGameplayStatics::GetPlayerPawn(ContextActor->GetWorld(), 0)) {
            PName = P->GetName();
        }
        BaseString = BaseString.Replace(TEXT("{PlayerName}"), *PName);
    }

    return FText::FromString(BaseString);
}
