#include "Core/AlsasuaOutcomeSystem.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Core/AlsasuaSaveGame.h"
#include "Kismet/GameplayStatics.h"

FAlsasuaMissionResult UAlsasuaOutcomeSystem::EvaluateManifestation()
{
    FAlsasuaMissionResult Result;
    UWorld* World = GetWorld();
    if (!World) return Result;

    UAlsasuaCrowdSentiment* Sentiment = World->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (!Sentiment) return Result;

    float FinalSupport = Sentiment->PopularSupport;
    float FinalTension = Sentiment->GlobalTension;

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
    UWorld* World = GetWorld();
    if (!World) return;

    // Evaluar hitos reales de la partida.
    UAlsasuaCrowdSentiment* Sentiment = World->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (Sentiment)
    {
        if (Sentiment->PopularSupport > 80.f)
            OutResult.AccomplishedFeats.Add(TEXT("Apoyo Popular Superior (80%+)"));
        if (Sentiment->GlobalTension < 0.3f)
            OutResult.AccomplishedFeats.Add(TEXT("Tensión Controlada (baja violencia)"));

        if (OutResult.bSuccess && Sentiment->PopularSupport > 60.f)
            OutResult.AccomplishedFeats.Add(TEXT("Mayoría de apoyo popular lograda"));
    }

    if (OutResult.bSuccess)
        OutResult.AccomplishedFeats.Add(TEXT("Manifestación completada con éxito"));
}

void UAlsasuaOutcomeSystem::FinalizeSession()
{
    FAlsasuaMissionResult FinalData = EvaluateManifestation();

    UDEPRECATED_AlsasuaSaveGame* Save = Cast<UDEPRECATED_AlsasuaSaveGame>(UGameplayStatics::CreateSaveGameObject(UDEPRECATED_AlsasuaSaveGame::StaticClass()));
    if (Save)
    {
        UWorld* World = GetWorld();
        if (World)
        {
            APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
            if (PC && PC->GetPawn())
            {
                Save->PlayerLocation = PC->GetPawn()->GetActorLocation();
                Save->PlayerRotation = PC->GetPawn()->GetActorRotation();
            }
        }

        Save->CompletedMissionIDs.Add(FinalData.bSuccess ? TEXT("manifestacion_success") : TEXT("manifestacion_fail"));
        if (!UGameplayStatics::SaveGameToSlot(Save, TEXT("AltsasuSlot"), 0))
        {
            UE_LOG(LogTemp, Error, TEXT("OUTCOME: Error al guardar juego en slot AltsasuSlot"));
        }
    }

    UE_LOG(LogTemp, Warning, TEXT("OUTCOME: Sesión Finalizada. Resultado: %s"), *FinalData.FinalVerdict.ToString());
}
