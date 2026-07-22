#include "Visuals/AlsasuaCinemaDirector.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"

void UAlsasuaCinemaDirector::Tick(float DeltaTime)
{
    UAlsasuaCrowdSentiment* Sentiment = GetWorld()->GetSubsystem<UAlsasuaCrowdSentiment>();
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
    // Aquí se inyectarían valores al PostProcessVolume global (Aberración cromática, grano)
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
