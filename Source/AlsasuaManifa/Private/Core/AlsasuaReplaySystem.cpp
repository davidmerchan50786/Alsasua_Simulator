#include "Core/AlsasuaReplaySystem.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"

void UAlsasuaReplaySystem::Tick(float DeltaTime)
{
    if (bIsRecording)
    {
        CaptureFrame();
    }
}

void UAlsasuaReplaySystem::StartRecording()
{
    CurrentSession.Empty();
    bIsRecording = true;
    UE_LOG(LogTemp, Warning, TEXT("REPLAY: Grabación de sesión iniciada."));
}

void UAlsasuaReplaySystem::StopRecording()
{
    bIsRecording = false;
    UE_LOG(LogTemp, Warning, TEXT("REPLAY: Grabación finalizada. %d frames capturados."), CurrentSession.Num());
}

void UAlsasuaReplaySystem::CaptureFrame()
{
    FReplaySnapshot NewFrame;
    NewFrame.TimeStamp = GetWorld()->GetTimeSeconds();

    UAlsasuaCrowdSentiment* Sentiment = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>();
    NewFrame.GlobalTension = Sentiment ? Sentiment->GlobalTension : 0.f;

    // Capturamos solo los actores críticos (Jugador, Guardias, Líderes) para eficiencia
    TArray<AActor*> KeyActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), KeyActors);

    for (AActor* Actor : KeyActors)
    {
        NewFrame.ActorStates.Add(Actor, Actor->GetActorTransform());
    }

    CurrentSession.Add(NewFrame);
}

void UAlsasuaReplaySystem::SaveReplayToFile(FString FileName)
{
    // Aquí se serializaría a un .sav o .json binario
    UE_LOG(LogTemp, Log, TEXT("REPLAY: Sesión guardada en %s"), *FileName);
}
