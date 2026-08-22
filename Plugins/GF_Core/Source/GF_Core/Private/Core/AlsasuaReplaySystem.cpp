#include "Core/AlsasuaReplaySystem.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "GameFramework/Character.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/PlatformFileManager.h"

void UAlsasuaReplaySystem::Tick(float DeltaTime)
{
    if (!bIsRecording)
    {
        return;
    }

    CaptureTimer += DeltaTime;
    if (CaptureTimer >= CaptureInterval)
    {
        CaptureTimer = 0.0f;
        CaptureFrame();
    }
}

void UAlsasuaReplaySystem::StartRecording()
{
    CurrentSession.Empty();
    bIsRecording = true;
    CaptureTimer = 0.0f;
    CacheRefreshTimer = 0.0f;
    RefreshActorCache();
    UE_LOG(LogTemp, Warning, TEXT("REPLAY: Grabación de sesión iniciada."));
}

void UAlsasuaReplaySystem::StopRecording()
{
    bIsRecording = false;
    UE_LOG(LogTemp, Warning, TEXT("REPLAY: Grabación finalizada. %d frames capturados."), CurrentSession.Num());
}

void UAlsasuaReplaySystem::RefreshActorCache()
{
    CachedKeyActors.Empty();
    TArray<AActor*> TempActors;
    UGameplayStatics::GetAllActorsOfClass(GetWorld(), ACharacter::StaticClass(), TempActors);
    for (AActor* Actor : TempActors)
    {
        if (ACharacter* Char = Cast<ACharacter>(Actor))
        {
            CachedKeyActors.Add(Char);
        }
    }
}

void UAlsasuaReplaySystem::CaptureFrame()
{
    UWorld* W = GetWorld();
    if (!W) return;

    FReplaySnapshot NewFrame;
    NewFrame.TimeStamp = W->GetTimeSeconds();

    UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
    NewFrame.GlobalTension = Sentiment ? Sentiment->GlobalTension : 0.f;

    // Refrescar cache periódicamente.
    CacheRefreshTimer += CaptureInterval;
    if (CacheRefreshTimer >= CacheRefreshInterval || CachedKeyActors.Num() == 0)
    {
        CacheRefreshTimer = 0.0f;
        RefreshActorCache();
    }

    // Capturar solo ubicaciones (FVector) en vez de TMap<AActor*, FTransform>
    // para evitar GC issues y reducir memoria.
    NewFrame.ActorLocations.Reserve(CachedKeyActors.Num());
    for (int32 i = CachedKeyActors.Num() - 1; i >= 0; --i)
    {
        if (IsValid(CachedKeyActors[i]))
        {
            NewFrame.ActorLocations.Add(CachedKeyActors[i]->GetActorLocation());
        }
        else
        {
            CachedKeyActors.RemoveAtSwap(i);
        }
    }

    CurrentSession.Add(NewFrame);
}

void UAlsasuaReplaySystem::SaveReplayToFile(FString FileName)
{
    if (CurrentSession.Num() == 0)
    {
        UE_LOG(LogTemp, Warning, TEXT("REPLAY: No hay datos para guardar."));
        return;
    }

    FString OutputPath = FPaths::ProjectSavedDir() / TEXT("Replays") / (FileName + TEXT(".json"));
    FPaths::NormalizeFilename(OutputPath);

    // Crear directorio si no existe.
    const FString ReplayDir = FPaths::ProjectSavedDir() / TEXT("Replays");
    if (!FPaths::DirectoryExists(ReplayDir))
    {
        FPlatformFileManager::Get().GetPlatformFile().CreateDirectoryTree(*ReplayDir);
    }

    TArray<FString> JsonLines;
    JsonLines.Add(TEXT("["));
    for (int32 i = 0; i < CurrentSession.Num(); ++i)
    {
        const FReplaySnapshot& Snap = CurrentSession[i];
        FString Frame = FString::Printf(TEXT("{\"Time\":%.2f,\"Tension\":%.3f,\"Actors\":["),
            Snap.TimeStamp, Snap.GlobalTension);

        bool bFirst = true;
        for (const FVector& Loc : Snap.ActorLocations)
        {
            if (!bFirst) Frame += TEXT(",");
            bFirst = false;
            Frame += FString::Printf(TEXT("{\"Loc\":\"%.0f,%.0f,%.0f\"}"), Loc.X, Loc.Y, Loc.Z);
        }
        Frame += TEXT("]}");
        JsonLines.Add(Frame);
    }
    JsonLines.Add(TEXT("]"));

    FString JsonContent = FString::Join(JsonLines, TEXT("\n"));
    if (FFileHelper::SaveStringToFile(JsonContent, *OutputPath))
    {
        UE_LOG(LogTemp, Log, TEXT("REPLAY: Sesión guardada en %s (%d frames)"), *OutputPath, CurrentSession.Num());
    }
    else
    {
        UE_LOG(LogTemp, Error, TEXT("REPLAY: Error al guardar en %s"), *OutputPath);
    }
}
