#include "Visuals/AlsasuaCinemaDirector.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Kismet/GameplayStatics.h"
#include "HAL/IConsoleManager.h"

// La escribe UAlsasuaSettingsWidget::ApplySettings, que hasta ahora la buscaba
// sin que existiera. Ver el bloque equivalente en AlsasuaCharacter.cpp.
static TAutoConsoleVariable<float> CVarIntensidadVibracion(
	TEXT("g.CameraShakeIntensity"), 1.0f,
	TEXT("Multiplicador de la vibración de cámara por tensión."), ECVF_Default);
#include "GameFramework/PlayerController.h"

void UAlsasuaCinemaDirector::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    CachedMPC = LoadObject<UMaterialParameterCollection>(
        nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
}

void UAlsasuaCinemaDirector::Tick(float DeltaTime)
{
    UWorld* W = GetWorld();
    if (!W) return;
    UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (!Sentiment) return;

    // 1. La sacudida de cámara depende de la tensión global (AAA+++)
    float TargetShake = Sentiment->GlobalTension * 0.5f;
    ShakeIntensity = FMath::FInterpTo(ShakeIntensity, TargetShake, DeltaTime, 1.0f);

    // 2. Si hay mucha tensión, aplicamos efectos de Post-Procesado (Simulado)
    UpdatePostProcessing(DeltaTime);
    CalculateCameraFocus(DeltaTime);
}

void UAlsasuaCinemaDirector::RegisterVisualInterest(FVector Location, float Importance, float Duration)
{
    // Lógica para que la cámara del jugador haga un "LookAt" suave hacia el disturbio
    ActiveInterests.Add(Location);
}

void UAlsasuaCinemaDirector::UpdatePostProcessing(float DeltaTime)
{
    UWorld* W = GetWorld();
    if (!W) return;
    UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (!Sentiment) return;

    const float Tension = Sentiment->GlobalTension;

    // Chromatic aberration scales with tension (0→0, 1→2.0).
    ChromaticAberration = FMath::FInterpTo(ChromaticAberration, Tension * 2.f, DeltaTime, 1.5f);

    // Film grain scales with tension (0→0, 1→1.0).
    FilmGrain = FMath::FInterpTo(FilmGrain, Tension, DeltaTime, 1.5f);

    // Apply to post-process volume via MaterialParameterCollection.
    if (!CachedMPC) return;

    UMaterialParameterCollectionInstance* Inst = W->GetParameterCollectionInstance(CachedMPC);
    if (!Inst) return;

    Inst->SetScalarParameterValue(FName("ChromaticAberration"), ChromaticAberration);
    Inst->SetScalarParameterValue(FName("FilmGrain"), FilmGrain);
}

void UAlsasuaCinemaDirector::CalculateCameraFocus(float DeltaTime)
{
    APlayerController* PC = UGameplayStatics::GetPlayerController(GetWorld(), 0);
    if (!PC || ShakeIntensity <= 0.1f) return;

    // Aquí se llamaba a ClientStartCameraShake(nullptr, ShakeIntensity). Con la
    // clase de shake a null esa función sale sin hacer nada, así que la tensión
    // no se transmitía: la vibración de cámara del juego no existía, y la barra
    // del menú de opciones que la gradúa tampoco tenía nada que graduar.
    //
    // Se aplica como micro-impulso de mirada, que es el mecanismo que ya usa
    // ArmasComponent para el retroceso y no depende de crear un UCameraShakeBase
    // ni de tener el asset. La amplitud es la que gradúa el ajuste.
    const float Ajuste = FMath::Clamp(CVarIntensidadVibracion.GetValueOnGameThread(), 0.f, 2.f);
    if (Ajuste <= 0.f) return;

    // Escalado por DeltaTime para que la vibración no dependa de los FPS, y
    // amplitud pequeña: esto es temblor de tensión, no un terremoto.
    const float A = ShakeIntensity * Ajuste * DeltaTime * 6.f;
    PC->AddPitchInput(FMath::FRandRange(-A, A));
    PC->AddYawInput(FMath::FRandRange(-A, A));
}
