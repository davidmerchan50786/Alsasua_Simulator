#include "Systemics/Festivals/FestivalSubsystem.h"
#include "Systems/Media/RadioSubsystem.h"

void UFestivalSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
    SetupRealDates();
}

void UFestivalSubsystem::SetupRealDates() {
    // Momotxorros (Carnaval): Aproximadamente Febrero
    Calendar.Add({2, 25, ETraditionType::Momotxorros});
    // San Pedro: 29 de Junio
    Calendar.Add({6, 29, ETraditionType::SanPedro});
    // Romería típica: Finales de Junio / Julio
    Calendar.Add({7, 2, ETraditionType::Romeria});
}

void UFestivalSubsystem::UpdateCalendar(float DeltaTime) {
    GameTime += DeltaTime;

    // Simulación rápida: 1 minuto real = 1 hora de juego (configurable)
    if(GameTime >= 60.f) {
        GameTime = 0.f;
        CurrentDay++;
        if(CurrentDay > 30) {
            CurrentDay = 1;
            CurrentMonth++;
            if(CurrentMonth > 12) CurrentMonth = 1;
        }
        CheckForFestivals();
    }
}

void UFestivalSubsystem::CheckForFestivals() {
    for(const auto& Event : Calendar) {
        if(Event.Day == CurrentDay && Event.Month == CurrentMonth) {
            StartFestival(Event.Festival);
        }
    }
}

void UFestivalSubsystem::StartFestival(ETraditionType Festival) {
    UWorld* W = GetWorld();
    URadioSubsystem* Radio = W->GetSubsystem<URadioSubsystem>();

    FString Msg_ES, Msg_EU;

    switch(Festival) {
        case ETraditionType::Momotxorros:
            Msg_ES = "¡Es tiempo de Momotxorros! El carnaval rural toma Altsasu.";
            Msg_EU = "Momotxorren garaia da! Herri inauteriak Altsasu hartzen du.";
            break;
        case ETraditionType::SanPedro:
            Msg_ES = "29 de Junio: San Pedro. Fiesta mayor en el pueblo.";
            Msg_EU = "Ekainak 29: San Pedro. Herriko festa nagusia.";
            break;
        default: break;
    }

    if(Radio) {
        Radio->TriggerUrgentNews(FText::FromString(Msg_ES), FText::FromString(Msg_EU), nullptr);
    }
}
