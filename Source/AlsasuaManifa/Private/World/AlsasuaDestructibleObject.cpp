#include "World/AlsasuaDestructibleObject.h"
#include "Components/PrimitiveComponent.h"
#include "AI/AlsasuaCrowdSentiment.h"

AAlsasuaDestructibleObject::AAlsasuaDestructibleObject()
{
    PrimaryActorTick.bCanEverTick = true;

    // Inicialización del Geometry Collection para Chaos Physics 5.4
    GeometryComponent = CreateDefaultSubobject<UPrimitiveComponent>(TEXT("GeometryComponent"));
    RootComponent = GeometryComponent;
}

void AAlsasuaDestructibleObject::BeginPlay()
{
    Super::BeginPlay();
}

void AAlsasuaDestructibleObject::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bBreakOnCrowdTension)
    {
        CheckCrowdPressure();
    }
}

void AAlsasuaDestructibleObject::ApplySysteimcDamage(float DamageAmount)
{
    Integrity -= DamageAmount;
    if (Integrity <= 0)
    {
        if (GeometryComponent)
        {
            GeometryComponent->SetSimulatePhysics(true);
            GeometryComponent->AddRadialImpulse(
                GetActorLocation(),
                500.f,
                5000.f,
                ERadialImpulseFalloff::RIF_Linear,
                true
            );
        }

        OnObjectDestroyed.Broadcast(this);
        UE_LOG(LogTemp, Warning, TEXT("%s destruido por impacto sistémico."), *GetName());
    }
}

void AAlsasuaDestructibleObject::CheckCrowdPressure()
{
    UWorld* W = GetWorld();
    if (!W) return;
    UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (Sentiment)
    {
        float LocalTension = (uint8)Sentiment->GetMoodAtLocation(GetActorLocation());

        // Si hay hostilidad extrema (Mood Hostile = 2+), el objeto sufre estrés físico
        if (LocalTension >= 2.0f)
        {
            ApplySysteimcDamage(0.5f); // Daño por "presión de la masa"
        }
    }
}
