#include "AlsasuaGameplayGameMode.h"
#include "AlsasuaGameplayHUD.h"
#include "DirectorArranque.h"
#include "TerrenoGenerado.h"
#include "ArranqueMundo.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/Pawn.h"
#include "GameFramework/PlayerStart.h"

AAlsasuaGameplayGameMode::AAlsasuaGameplayGameMode()
{
    HUDClass = AAlsasuaGameplayHUD::StaticClass();
    PrimaryActorTick.bCanEverTick = true;
}

void AAlsasuaGameplayGameMode::InitGame(const FString& MapName, const FString& Options, FString& ErrorMessage)
{
    Super::InitGame(MapName, Options, ErrorMessage);
    UE_LOG(LogTemp, Log, TEXT("AlsasuaGameplayGameMode: Inicializado en mapa %s"), *MapName);
}

void AAlsasuaGameplayGameMode::StartPlay()
{
    Super::StartPlay();

    UWorld* World = GetWorld();
    if (World && !ArranqueMundo::HayDirector)
    {
        World->SpawnActor<ADirectorArranque>(ADirectorArranque::StaticClass(), FVector::ZeroVector, FRotator::ZeroRotator);
        UE_LOG(LogTemp, Log, TEXT("AlsasuaGameplayGameMode: DirectorArranque spawneado (mundo real Alsasua)."));
    }

    bPendienteColocarJugador = true;

    HoraInicioWall = FPlatformTime::Seconds();
    if (GEngine)
    {
        GEngine->Exec(nullptr, TEXT("CsvProfile start"));
    }
}

void AAlsasuaGameplayGameMode::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);

    UWorld* World = GetWorld();
    if (World && (FPlatformTime::Seconds() - HoraInicioWall) > 240.0)
    {
        UE_LOG(LogTemp, Log, TEXT("AlsasuaGameplayGameMode: Wall 240s alcanzado, saliendo (game time %.1f)."), World->GetTimeSeconds());
        if (GEngine)
        {
            GEngine->Exec(nullptr, TEXT("CsvProfile stop"));
        }
        const FString CsvDir = FPaths::ProfilingDir() + TEXT("CSV/");
        for (int32 i = 0; i < 60 && !bCsvListo; ++i)
        {
            FPlatformProcess::Sleep(0.5f);
            TArray<FString> Files;
            IFileManager::Get().FindFiles(Files, *(CsvDir + TEXT("Profile(*.csv")), true, false);
            for (const FString& F : Files)
            {
                if (IFileManager::Get().FileSize(*(CsvDir + F)) > 0)
                {
                    bCsvListo = true;
                    break;
                }
            }
        }
        FGenericPlatformMisc::RequestExit(false);
        return;
    }

    if (!bPendienteColocarJugador)
    {
        return;
    }

    if (!World)
    {
        return;
    }

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    APawn* Pawn = PC ? PC->GetPawn() : nullptr;
    if (!Pawn)
    {
        // Sin PlayerStart en el mapa el pawn nunca spawnea (-game/servidor).
        // Garantizarlo: spawnear un start en Herriko Plaza y restartear.
        const double T = World->GetTimeSeconds();
        if (T > 3.0f && T < 3.5f)
        {
            const FVector Plaza(191800.f, 857000.f, 260.f);
            if (APlayerStart* PS = World->SpawnActor<APlayerStart>(APlayerStart::StaticClass(), Plaza, FRotator::ZeroRotator))
            {
                UE_LOG(LogTemp, Log, TEXT("AlsasuaGameplayGameMode: PlayerStart de respaldo en Herriko Plaza."));
            }
        }
        if (World->GetTimeSeconds() > 4.0f)
        {
            if (APlayerController* C = UGameplayStatics::GetPlayerController(World, 0))
            {
                RestartPlayer(C);
            }
        }
        return;
    }

    ATerrenoGenerado* Terreno = Cast<ATerrenoGenerado>(UGameplayStatics::GetActorOfClass(World, ATerrenoGenerado::StaticClass()));
    if (!Terreno)
    {
        return;
    }

    const FVector Plaza(191800.f, 857000.f, 0.f);
    const float Z = Terreno->AlturaEnMundo(Plaza.X, Plaza.Y) + 200.f;
    Pawn->SetActorLocation(FVector(Plaza.X, Plaza.Y, Z));
    PC->SetControlRotation(FRotator(0.f, -90.f, 0.f));
    bPendienteColocarJugador = false;
    UE_LOG(LogTemp, Log, TEXT("AlsasuaGameplayGameMode: Jugador colocado en Herriko Plaza (%.1f, %.1f, %.1f)."), Plaza.X, Plaza.Y, Z);
}