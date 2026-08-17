#include "World/AlsasuaDestructibleObject.h"
#include "Components/StaticMeshComponent.h"
#include "AI/AlsasuaCrowdSentiment.h"

AAlsasuaDestructibleObject::AAlsasuaDestructibleObject()
{
    PrimaryActorTick.bCanEverTick = true;

    // UStaticMeshComponent, no UPrimitiveComponent a secas: el segundo es una
    // clase abstracta —no tiene malla, ni geometría, ni nada que simular—, así
    // que el SetSimulatePhysics y el AddRadialImpulse de abajo se aplicaban a
    // algo que no puede moverse ni verse. La malla se asigna en el editor o en
    // el Blueprint que derive de esto; el tipo del miembro sigue siendo
    // UPrimitiveComponent, que es la API que usa el resto del fichero.
    GeometryComponent = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("GeometryComponent"));
    RootComponent = GeometryComponent;
}

void AAlsasuaDestructibleObject::BeginPlay()
{
    Super::BeginPlay();
}

void AAlsasuaDestructibleObject::Tick(float DeltaTime)
{
    Super::Tick(DeltaTime);

    if (bBreakOnCrowdTension && !bDestruido)
    {
        CheckCrowdPressure(DeltaTime);
    }
}

void AAlsasuaDestructibleObject::AplicarDanoSistemico(float DamageAmount)
{
    // Una vez roto, roto. Antes no había guarda: Integrity se quedaba en
    // negativo, la condición <= 0 seguía cumpliéndose y cada frame se volvían a
    // llamar SetSimulatePhysics y AddRadialImpulse y se volvía a emitir
    // OnObjectDestroyed. Lo que debía ser un aviso de "esto se ha roto" era un
    // aviso a 60 Hz para siempre, y el impulso radial mantenía la pieza
    // saltando en el sitio.
    if (bDestruido) return;

    Integrity -= DamageAmount;
    if (Integrity > 0.f) return;

    bDestruido = true;
    SetActorTickEnabled(false);

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
    UE_LOG(LogTemp, Warning, TEXT("AlsasuaDestructibleObject: %s destruido por presión de la multitud."), *GetName());
}

void AAlsasuaDestructibleObject::CheckCrowdPressure(float DeltaTime)
{
    UWorld* W = GetWorld();
    if (!W) return;

    const UAlsasuaCrowdSentiment* Sentiment = W->GetSubsystem<UAlsasuaCrowdSentiment>();
    if (!Sentiment) return;

    // Hostile en adelante (Hostile=2, Panic=3): a partir de ahí la masa empuja.
    if (Sentiment->GetMoodAtLocation(GetActorLocation()) < ECrowdMood::Hostile) return;

    // Por segundo, no por frame. Estaba en 0.5 fijo por Tick, así que el daño
    // dependía de los FPS: a 60 una pieza de 100 de integridad aguantaba 3,3 s
    // y a 120 la mitad. Con DesgastePorSegundo el aguante es el mismo en todas
    // las máquinas, que además es lo que hace comparables dos perfilados.
    AplicarDanoSistemico(DesgastePorSegundo * DeltaTime);
}
