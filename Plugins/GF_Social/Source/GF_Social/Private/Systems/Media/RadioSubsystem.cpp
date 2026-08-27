#include "Systems/Media/RadioSubsystem.h"
#include "WantedSubsystem.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/GameInstance.h"

void URadioSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* W = GetWorld())
    {
        if (UGameInstance* GI = W->GetGameInstance())
        {
            if (UWantedSubsystem* Wanted = GI->GetSubsystem<UWantedSubsystem>())
                Wanted->OnEstrellasCambia.AddDynamic(this, &URadioSubsystem::OnWantedLevelChange);
        }
    }
}

void URadioSubsystem::OnWantedLevelChange(int32 Nivel)
{
    if (Nivel < 1) return;

    FText Headline;
    FText Body;

    switch (Nivel)
    {
    case 1:
        Headline = NSLOCTEXT("Radio", "W1", "Se reportan disturbios menores en el casco urbano. Policía local investigando.");
        Body = NSLOCTEXT("Radio", "W1B", "Fuentes policiales confirman que se han registrado incidentes aislados. Se recomienda precaución.");
        break;
    case 2:
        Headline = NSLOCTEXT("Radio", "W2", "ALERTA: Operación policial en curso. Se recomienda evitar la zona.");
        Body = NSLOCTEXT("Radio", "W2B", "Varias patrullas han sido desplegadas en respuesta a los altercados. La situación está bajo control according to fuentes oficiales.");
        break;
    case 3:
        Headline = NSLOCTEXT("Radio", "W3", "URGENTE: Refuerzos de la Guardia Civil desplegados. Zona parcialmente acordonada.");
        Body = NSLOCTEXT("Radio", "W3B", "Helicópteros de vigilancia sobrevuelan la zona. Se solicita a la población permanecer en sus domicilios.");
        break;
    case 4:
        Headline = NSLOCTEXT("Radio", "W4", "SITUACIÓN CRÍTICA: Estado de alerta máximo. Operaciones antidisturbios desplegadas.");
        Body = NSLOCTEXT("Radio", "W4B", "Las fuerzas de seguridad han establecido perímetros de seguridad. Se reportan enfrentamientos en varios puntos de la ciudad.");
        break;
    case 5:
        Headline = NSLOCTEXT("Radio", "W5", "EMERGENCIA: Se declara estado de excepción. Toque de facto en la zona.");
        Body = NSLOCTEXT("Radio", "W5B", "Todas las unidades policiales están en alerta máxima. Se recomienda encarecidamente no salir a la calle. La situación es extremadamente peligrosa.");
        break;
    default:
        Headline = FText::Format(
            NSLOCTEXT("Radio", "WDefault", "Atención: nivel de alerta {0} mantenido. Refuerzos en la zona."),
            FText::AsNumber(Nivel));
        break;
    }

    TriggerUrgentNews(Headline, Body);
}

void URadioSubsystem::TriggerUrgentNews(FText Headline, FText Body, USoundBase* Voice) {
    FRadioNewsClip News;
    News.NewsHeadline = Headline;
    News.NewsBody = Body;
    News.VoiceClip = Voice;

    // Play the voice clip if provided.
    if (Voice && GetWorld())
    {
        APlayerController* PC = GetWorld()->GetFirstPlayerController();
        if (PC)
        {
            UGameplayStatics::PlaySound2D(PC, Voice, 0.8f);
        }
    }

    OnRadioUpdate.Broadcast(News);
}

void URadioSubsystem::AddClandestineMessage(FText Sender, FText Message) {
    FRadioNewsClip Msg;
    Msg.NewsHeadline = Sender;
    Msg.NewsBody = Message;

    OnTelegramUpdate.Broadcast(Msg);
}
