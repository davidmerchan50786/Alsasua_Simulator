#include "Visuals/AlsasuaCinemaDirector.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Kismet/GameplayStatics.h"
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
    if (PC && ShakeIntensity > 0.1f)
    {
        // Aplicamos micro-vibraciones al control de la cámara para transmitir tensión
        PC->ClientStartCameraShake(nullptr, ShakeIntensity); 
    }
}
