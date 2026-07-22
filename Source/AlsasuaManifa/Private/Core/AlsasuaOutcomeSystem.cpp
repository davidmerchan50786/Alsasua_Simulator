#include "Core/AlsasuaOutcomeSystem.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Core/AlsasuaSaveGame.h"
#include "Kismet/GameplayStatics.h"

FAlsasuaMissionResult UAlsasuaOutcomeSystem::EvaluateManifestation()
{
    FAlsasuaMissionResult Result;
    UAlsasuaCrowdSentiment* Sentiment = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>();

    if (!Sentiment) return Result;

    float FinalSupport = Sentiment->PopularSupport;
    float FinalTension = Sentiment->GlobalTension;

    // Lógica de éxito AAA+++
    if (FinalSupport > 60.f && FinalTension < 0.8f)
    {
        Result.bSuccess = true;
        Result.FinalVerdict = FText::FromString("¡ÉXITO HISTÓRICO! Alsasua se ha unido pacíficamente.");
        Result.BonusCredits = 1000;
    }
    else if (FinalSupport > 30.f)
    {
        Result.bSuccess = true;
        Result.FinalVerdict = FText::FromString("RESISTENCIA LOGRADA. El mensaje ha calado a pesar de la tensión.");
        Result.BonusCredits = 500;
    }
    else
    {
        Result.bSuccess = false;
        Result.FinalVerdict = FText::FromString("FRACASO. La dispersión y el caos han silenciado la protesta.");
        Result.BonusCredits = 0;
    }

    ProcessFeats(Result);
    return Result;
}

void UAlsasuaOutcomeSystem::ProcessFeats(FAlsasuaMissionResult& OutResult)
{
    // Aquí se chequearían hitos específicos logrados durante la partida
    OutResult.AccomplishedFeats.Add("Pancarta Principal Desplegada");
    OutResult.AccomplishedFeats.Add("Evacuación del Furgón completada");
}

void UAlsasuaOutcomeSystem::FinalizeSession()
{
    FAlsasuaMissionResult FinalData = EvaluateManifestation();

    // Guardado persistente
    UAlsasuaSaveGame* Save = Cast<UAlsasuaSaveGame>(UGameplayStatics::CreateSaveGameObject(UAlsasuaSaveGame::StaticClass()));
    if (Save)
    {
        // Actualizar estadísticas globales del jugador en el SaveGame
        // UGameplayStatics::SaveGameToSlot(Save, "AltsasuSlot", 0);
    }

    UE_LOG(LogTemp, Warning, TEXT("OUTCOME: Sesión Finalizada. Resultado: %s"), *FinalData.FinalVerdict.ToString());
}
