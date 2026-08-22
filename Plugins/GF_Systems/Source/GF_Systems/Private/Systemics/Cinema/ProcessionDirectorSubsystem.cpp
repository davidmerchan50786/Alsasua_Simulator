#include "Systemics/Cinema/ProcessionDirectorSubsystem.h"
#include "Systems/Media/RadioSubsystem.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Systemics/Urban/UrbanStateSubsystem.h"
#include "Systems/DeepState/DeepStateSubsystem.h"
#include "Systemics/Missions/DeepStateMissionData.h"
#include "Kismet/GameplayStatics.h"

void UProcessionDirectorSubsystem::StartProcessionEvent() {
    UWorld* W = GetWorld();
    if (!W) return;
    URadioSubsystem* Radio = W->GetSubsystem<URadioSubsystem>();

    if(Radio) {
        Radio->TriggerUrgentNews(
            FText::FromString("COBERTURA INTEGRAL: Comienza la Procesión de San Pedro. Tensión política máxima."),
            FText::FromString("ESTALDURA OSOA: San Pedroko Prozesioa hasi da. Tentsio politiko handia."),
            nullptr
        );
    }

    if(UDeepStateSubsystem* DSS = W->GetSubsystem<UDeepStateSubsystem>()) {
        DSS->LaunchCovertOp("OperacionFinal", EDeepStateOp::EvidencePlanting);
    }
}

void UProcessionDirectorSubsystem::PlayerAction_ExposeCloacas() {
    CurrentOutcome = EProcessionOutcome::PublicJustice;

    UWorld* W = GetWorld();
    if (!W) return;

    if(USocialMediaSubsystem* Social = W->GetSubsystem<USocialMediaSubsystem>()) {
        Social->AddFollowers(50000);
    }

    if(UUrbanStateSubsystem* Urban = W->GetSubsystem<UUrbanStateSubsystem>()) {
        Urban->IncreaseTension("Global", -50.f);
    }

    if(URadioSubsystem* Radio = W->GetSubsystem<URadioSubsystem>()) {
        Radio->TriggerUrgentNews(
            FText::FromString("¡ESCÁNDALO! Documentos filtrados en directo implican a altos cargos en un montaje."),
            FText::FromString("ISLADA! Zuzenean filtratutako dokumentuek goi-karguak implikatzen dituzte muntaia batean."),
            nullptr
        );
    }
}

void UProcessionDirectorSubsystem::TriggerBombFailure() {
    CurrentOutcome = EProcessionOutcome::BloodySunday;

    UWorld* W = GetWorld();
    if (!W) return;

    OnPlayerStatsModified.Broadcast(5.f, -80.f);

    if (UUrbanStateSubsystem* Urban = W->GetSubsystem<UUrbanStateSubsystem>()) {
        Urban->IncreaseTension("Global", 100.f);
    }

    if (URadioSubsystem* Radio = W->GetSubsystem<URadioSubsystem>()) {
        Radio->TriggerUrgentNews(
            FText::FromString("TRAGEDIA: Explosión en la Procesión. Víctimas mortales. Estado de sitio declarado."),
            FText::FromString("TRISTEERA: Leherketa San Pedroko Prozesioan. Hildakoak. Alert egoera deklaratu da."),
            nullptr
        );
    }

    if (UDeepStateSubsystem* DSS = W->GetSubsystem<UDeepStateSubsystem>()) {
        DSS->LaunchCovertOp("RepresionPostAtentado", EDeepStateOp::EvidencePlanting);
    }

    UpdateCityStateByOutcome();
}

void UProcessionDirectorSubsystem::UpdateCityStateByOutcome() {
}
