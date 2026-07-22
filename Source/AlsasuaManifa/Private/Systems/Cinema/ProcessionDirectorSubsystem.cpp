#include "Systems/Cinema/ProcessionDirectorSubsystem.h"
#include "Systems/Media/RadioSubsystem.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Systems/Urban/UrbanStateSubsystem.h"
#include "Systems/DeepState/DeepStateSubsystem.h"
#include "Systems/Missions/DeepStateMissionData.h"

void UProcessionDirectorSubsystem::StartProcessionEvent() {
    UWorld* W = GetWorld();
    URadioSubsystem* Radio = W->GetSubsystem<URadioSubsystem>();

    // 1. Notificación masiva bilingüe
    if(Radio) {
        Radio->TriggerUrgentNews(
            FText::FromString("COBERTURA INTEGRAL: Comienza la Procesión de San Pedro. Tensión política máxima."),
            FText::FromString("ESTALDURA OSOA: San Pedroko Prozesioa hasi da. Tentsio politiko handia."),
            nullptr
        );
    }

    // 2. Activar a las cloacas en modo "Operación Final"
    if(UDeepStateSubsystem* DSS = W->GetSubsystem<UDeepStateSubsystem>()) {
        DSS->LaunchCovertOp("OperacionFinal", EDeepStateOp::EvidencePlanting);
    }
}

void UProcessionDirectorSubsystem::PlayerAction_ExposeCloacas() {
    CurrentOutcome = EProcessionOutcome::PublicJustice;

    if(USocialMediaSubsystem* Social = GetWorld()->GetSubsystem<USocialMediaSubsystem>()) {
        Social->AddFollowers(50000); // Viralidad absoluta
    }

    if(UUrbanStateSubsystem* Urban = GetWorld()->GetSubsystem<UUrbanStateSubsystem>()) {
        Urban->IncreaseTension("Global", -50.f); // El pueblo se une, la tensión baja al saberse la verdad
    }

    // Noticia final de Radio
    if(URadioSubsystem* Radio = GetWorld()->GetSubsystem<URadioSubsystem>()) {
        Radio->TriggerUrgentNews(
            FText::FromString("¡ESCÁNDALO! Documentos filtrados en directo implican a altos cargos en un montaje."),
            FText::FromString("ISLADA! Zuzenean filtratutako dokumentuek goi-karguak implikatzen dituzte muntaia batean."),
            nullptr
        );
    }
}

void UProcessionDirectorSubsystem::TriggerBombFailure() {
    CurrentOutcome = EProcessionOutcome::BloodySunday;
    // Efectos de tragedia: WantedLevel al máximo, apoyo popular roto, estado de sitio.
}
