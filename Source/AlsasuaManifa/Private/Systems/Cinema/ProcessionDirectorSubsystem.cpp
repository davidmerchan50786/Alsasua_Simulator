#include "Systems/Cinema/ProcessionDirectorSubsystem.h"
#include "Systems/Media/RadioSubsystem.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Systems/Urban/UrbanStateSubsystem.h"
#include "Systems/DeepState/DeepStateSubsystem.h"
#include "Systems/Missions/DeepStateMissionData.h"
#include "AlsasuaCharacter.h"
#include "AlsasuaAttributeSet.h"
#include "Kismet/GameplayStatics.h"

void UProcessionDirectorSubsystem::StartProcessionEvent() {
    UWorld* W = GetWorld();
    if (!W) return;
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

    UWorld* W = GetWorld();
    if (!W) return;

    if(USocialMediaSubsystem* Social = W->GetSubsystem<USocialMediaSubsystem>()) {
        Social->AddFollowers(50000); // Viralidad absoluta
    }

    if(UUrbanStateSubsystem* Urban = W->GetSubsystem<UUrbanStateSubsystem>()) {
        Urban->IncreaseTension("Global", -50.f); // El pueblo se une, la tensión baja al saberse la verdad
    }

    // Noticia final de Radio
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

    // 1. WantedLevel al máximo + apoyo popular destruido (vía AttributeSet del jugador).
    if (AAlsasuaCharacter* Player = Cast<AAlsasuaCharacter>(UGameplayStatics::GetPlayerCharacter(W, 0)))
    {
        if (UAlsasuaAttributeSet* Attr = Player->GetAttributeSet())
        {
            Attr->SetWantedLevel(5.f);
            Attr->SetPopularSupport(FMath::Max(0.f, Attr->GetPopularSupport() - 80.f));
        }
    }

    // 2. Tensión global al máximo.
    if (UUrbanStateSubsystem* Urban = W->GetSubsystem<UUrbanStateSubsystem>()) {
        Urban->IncreaseTension("Global", 100.f);
    }

    // 3. Noticia de radio: Dolor y caos.
    if (URadioSubsystem* Radio = W->GetSubsystem<URadioSubsystem>()) {
        Radio->TriggerUrgentNews(
            FText::FromString("TRAGEDIA: Explosión en la Procesión. Víctimas mortales. Estado de sitio declarado."),
            FText::FromString("TRISTEERA: Leherketa San Pedroko Prozesioan. Hildakoak. Alert egoera deklaratu da."),
            nullptr
        );
    }

    // 4. Deep State: Operación de represión.
    if (UDeepStateSubsystem* DSS = W->GetSubsystem<UDeepStateSubsystem>()) {
        DSS->LaunchCovertOp("RepresionPostAtentado", EDeepStateOp::EvidencePlanting);
    }

    UpdateCityStateByOutcome();
}

void UProcessionDirectorSubsystem::UpdateCityStateByOutcome() {
}
