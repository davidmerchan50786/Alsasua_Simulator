#include "Systemics/Events/EventManagerSubsystem.h"
#include "Systems/Social/SocialMediaSubsystem.h"
#include "Systemics/Urban/UrbanStateSubsystem.h"
#include "Systems/Forensics/EvidenceComponent.h"
#include "Systems/Media/RadioSubsystem.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Kismet/GameplayStatics.h"

void UEventManagerSubsystem::Initialize(FSubsystemCollectionBase& Collection) {
    Super::Initialize(Collection);
}

void UEventManagerSubsystem::TickDirector(float DeltaTime) {
    TickFestivals(DeltaTime);

    CheckTimer += DeltaTime;
    if (CheckTimer >= CheckInterval) {
        CheckTimer = 0.f;
        EvaluateWorldState();
    }
}

void UEventManagerSubsystem::EvaluateWorldState() {
    UWorld* World = GetWorld();
    if (!World) return;

    float Tension = 0.f;
    int32 Followers = 0;

    if (UUrbanStateSubsystem* Urban = World->GetSubsystem<UUrbanStateSubsystem>()) {
        FSectorState GlobalState = Urban->GetSectorState(FName("Global"));
        Tension = GlobalState.TensionLevel;
    }
    if (USocialMediaSubsystem* Social = World->GetSubsystem<USocialMediaSubsystem>()) {
        Followers = Social->TotalFollowers;
    }

    // High tension: broadcast news via radio and increase global tension.
    if (Tension > 80.f) {
        FWorldEventData HighTensionEvent;
        HighTensionEvent.EventID = FName("TensionCritica");
        HighTensionEvent.EventAnnounceMessage = FText::FromString("La tensión social ha alcanzado niveles críticos. Incidentes reportados en varios barrios.");
        HighTensionEvent.Probability = 0.9f;
        OnEventTriggered(HighTensionEvent);
        OnDirectorAction.Broadcast(HighTensionEvent.EventAnnounceMessage);

        // Push breaking news to radio.
        if (URadioSubsystem* Radio = World->GetSubsystem<URadioSubsystem>()) {
            Radio->TriggerUrgentNews(
                FText::FromString(TEXT("URGENTE")),
                HighTensionEvent.EventAnnounceMessage);
        }

        // Escalate crowd sentiment.
        if (UAlsasuaCrowdSentiment* Sentiment = World->GetSubsystem<UAlsasuaCrowdSentiment>()) {
            Sentiment->GlobalTension = FMath::Min(1.0f, Sentiment->GlobalTension + 0.2f);
        }
    }

    // Viral event: broadcast clandestine message.
    if (Followers > 50000) {
        FWorldEventData ViralEvent;
        ViralEvent.EventID = FName("Viralizacion");
        ViralEvent.EventAnnounceMessage = FText::FromString("La resistencia se ha viralizado. Nuevos voluntarios se suman al movimiento.");
        ViralEvent.Probability = 0.8f;
        OnEventTriggered(ViralEvent);
        OnDirectorAction.Broadcast(ViralEvent.EventAnnounceMessage);

        if (URadioSubsystem* Radio = World->GetSubsystem<URadioSubsystem>()) {
            Radio->AddClandestineMessage(
                FText::FromString(TEXT("La Resistance")),
                FText::FromString(TEXT("¡El pueblo se levanta! Cada like es un voto más.")));
        }
    }

    // Calm growth: positive feedback.
    if (Tension < 20.f && Followers > 10000) {
        FWorldEventData CalmGrowthEvent;
        CalmGrowthEvent.EventID = FName("CrecimientoPacífico");
        CalmGrowthEvent.EventAnnounceMessage = FText::FromString("La resistencia crece en calma. Apoyo popular consolidado.");
        CalmGrowthEvent.Probability = 0.6f;
        OnEventTriggered(CalmGrowthEvent);
    }
}

void UEventManagerSubsystem::HandleEvidenceCollected(AActor* Owner, FName Tag) {
    if (UWorld* World = GetWorld()) {
        if (UUrbanStateSubsystem* UrbanSS = World->GetSubsystem<UUrbanStateSubsystem>()) {
            UrbanSS->IncreaseTension("Global", 15.0f);
            OnDirectorAction.Broadcast(FText::FromString("¡ALERTA! La Guardia Civil ha recuperado pruebas en la escena. El nivel de búsqueda ha subido."));
        }
        // Also push evidence news to radio.
        if (URadioSubsystem* Radio = World->GetSubsystem<URadioSubsystem>()) {
            Radio->TriggerUrgentNews(
                FText::FromString(TEXT("Noticias de última hora")),
                FText::FromString(TEXT("Fuentes policiales confirman que se han incautado pruebas comprometedoras en la zona.")));
        }
    }
}

// Reloj de juego acelerado: 1 min real = 1 hora de juego, meses de 30 dias.
void UEventManagerSubsystem::TickFestivals(float DeltaTime)
{
	FestivoGameHours += DeltaTime / 60.f;
	if (FestivoGameHours < 1.f) return;
	FestivoGameHours = 0.f;

	++FestivoDay;
	if (FestivoDay > 30) { FestivoDay = 1; ++FestivoMonth; }
	if (FestivoMonth > 12) FestivoMonth = 1;

	CheckFestivalDay();
}

void UEventManagerSubsystem::CheckFestivalDay()
{
	if (FestivoMonth == 2 && FestivoDay == 25)
	{
		StartFestival(
			FText::FromString(TEXT("Momotxorros")),
			FText::FromString(TEXT("¡Es tiempo de Momotxorros! El carnaval rural toma Altsasu.")));
	}
	else if (FestivoMonth == 6 && FestivoDay == 29)
	{
		StartFestival(
			FText::FromString(TEXT("San Pedro")),
			FText::FromString(TEXT("29 de junio: San Pedro. Fiesta mayor en el pueblo.")));
	}
	else if (FestivoMonth == 7 && FestivoDay == 2)
	{
		StartFestival(
			FText::FromString(TEXT("Romería")),
			FText::FromString(TEXT("Romería tradicional: romeros y verbena en la campa.")));
	}
}

void UEventManagerSubsystem::StartFestival(const FText& Nombre, const FText& Descripcion)
{
	UWorld* World = GetWorld();
	if (!World) return;

	FWorldEventData Data;
	Data.EventID = FName(*Nombre.ToString());
	Data.EventAnnounceMessage = Descripcion;
	Data.Probability = 1.f;
	OnEventTriggered(Data);
	OnDirectorAction.Broadcast(Descripcion);

	// Las fiestas llenan la plaza de gente: sube el apoyo popular y baja la tension.
	if (USocialMediaSubsystem* Social = World->GetSubsystem<USocialMediaSubsystem>())
	{
		Social->AddFollowers(3000);
	}
	if (UUrbanStateSubsystem* Urban = World->GetSubsystem<UUrbanStateSubsystem>())
	{
		Urban->IncreaseTension("Global", -20.f);
	}

	if (URadioSubsystem* Radio = World->GetSubsystem<URadioSubsystem>())
	{
		Radio->TriggerUrgentNews(Nombre, Descripcion);
	}
}
