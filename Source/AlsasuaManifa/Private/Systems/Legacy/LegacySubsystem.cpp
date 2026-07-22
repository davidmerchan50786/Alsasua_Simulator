#include "Systems/Legacy/LegacySubsystem.h"
#include "Systems/Ethics/KarmaSubsystem.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaAttributeSet.h"
#include "Kismet/GameplayStatics.h"

void ULegacySubsystem::TriggerMissionEnd() {
    FEndGameCinematicData Data;
    UKarmaSubsystem* Karma = GetWorld()->GetSubsystem<UKarmaSubsystem>();

    float FinalSupport = 0.f;
    APawn* Player = UGameplayStatics::GetPlayerPawn(GetWorld(), 0);
    if (AAlsasuaCharacter* Char = Cast<AAlsasuaCharacter>(Player)) {
        if (const UAlsasuaAttributeSet* Attr = Char->GetAttributeSet()) {
            FinalSupport = Attr->GetPopularSupport();
        }
    }

    // Lógica de final por impacto social y ética
    if (FinalSupport > 75.f) {
        Data.CinematicTitle = NSLOCTEXT("AAA", "EndSuccess", "LA PLAZA ES DEL PUEBLO");
        Data.SequenceToPlay = "LS_Victory_Dawn";
        Data.GradingColor = FLinearColor(1.0f, 0.8f, 0.4f); // Dorado/Esperanza
    }
    else if (FinalSupport < 25.f) {
        Data.CinematicTitle = NSLOCTEXT("AAA", "EndDefeat", "EL ECO DE LAS CALLES VACÍAS");
        Data.SequenceToPlay = "LS_Defeat_Rain";
        Data.GradingColor = FLinearColor(0.2f, 0.2f, 0.5f); // Azul/Tristeza
    }
    else {
        Data.CinematicTitle = NSLOCTEXT("AAA", "EndNeutral", "ALTSASU: EL DÍA DESPUÉS");
        Data.SequenceToPlay = "LS_Neutral_Default";
        Data.GradingColor = FLinearColor(0.5f, 0.5f, 0.5f); // Gris/Incertidumbre
    }

    OnMissionEndingReached.Broadcast(Data);
}
