#include "Systemics/Missions/DeepStateMissionData.h"
#include "Systems/Media/RadioSubsystem.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Systems/DeepState/DeepStateSubsystem.h"
#include "Kismet/GameplayStatics.h"

void UDeepStateMissionLogic::ExecuteExposeLeak(UWorld* World, const FMisionCloacas& Mission) {
    if(!World) return;

    // 1. Notificar a la Radio (Bilingüe)
    if(URadioSubsystem* Radio = World->GetSubsystem<URadioSubsystem>()) {
        Radio->TriggerUrgentNews(Mission.SuccessRadioHeadline_ES, FText::FromString("Documentos filtrados revelan operativos encubiertos."), nullptr);
    }

    // 2. Sabotear la operación en curso de las cloacas
    if(UDeepStateSubsystem* DSS = World->GetSubsystem<UDeepStateSubsystem>()) {
        DSS->CounterOperation(Mission.TargetOperationID, 100.f);
    }

    // 3. Impacto en Redes Sociales
    if(USocialMediaSubsystem* Social = World->GetSubsystem<USocialMediaSubsystem>()) {
        Social->AddFollowers(1000);
    }
}
