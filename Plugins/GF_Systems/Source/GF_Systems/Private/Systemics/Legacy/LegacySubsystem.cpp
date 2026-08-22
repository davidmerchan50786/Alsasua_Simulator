#include "Systemics/Legacy/LegacySubsystem.h"
#include "Systemics/Ethics/KarmaSubsystem.h"
#include "Kismet/GameplayStatics.h"

void ULegacySubsystem::TriggerMissionEnd() {
    UWorld* W = GetWorld();
    if (!W) return;

    FEndGameCinematicData Data;
    UKarmaSubsystem* Karma = W->GetSubsystem<UKarmaSubsystem>();

    if (FinalPopularSupport > 75.f) {
        Data.CinematicTitle = NSLOCTEXT("AAA", "EndSuccess", "LA PLAZA ES DEL PUEBLO");
        Data.SequenceToPlay = "LS_Victory_Dawn";
        Data.GradingColor = FLinearColor(1.0f, 0.8f, 0.4f);
    }
    else if (FinalPopularSupport < 25.f) {
        Data.CinematicTitle = NSLOCTEXT("AAA", "EndDefeat", "EL ECO DE LAS CALLES VACÍAS");
        Data.SequenceToPlay = "LS_Defeat_Rain";
        Data.GradingColor = FLinearColor(0.2f, 0.2f, 0.5f);
    }
    else {
        Data.CinematicTitle = NSLOCTEXT("AAA", "EndNeutral", "ALTSASU: EL DÍA DESPUÉS");
        Data.SequenceToPlay = "LS_Neutral_Default";
        Data.GradingColor = FLinearColor(0.5f, 0.5f, 0.5f);
    }

    OnMissionEndingReached.Broadcast(Data);
}
