#include "Items/ResistanceTool.h"
#include "AlsasuaTypes.h"
#include "Engine/OverlapResult.h"
#include "AI/AlsasuaCrowdSentiment.h"
#include "Kismet/GameplayStatics.h"
#include "NiagaraFunctionLibrary.h"
#include "NiagaraSystem.h"
#include "Sound/SoundBase.h"
#include "Engine/World.h"
#include "DrawDebugHelpers.h"

AResistanceTool::AResistanceTool()
{
}

void AResistanceTool::BeginPlay()
{
    Super::BeginPlay();
    NSSlingshot = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_SlingshotImpact.NS_SlingshotImpact"));
    SSlingshot = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Tirachinas.SC_Tirachinas"));
    NSSmoke = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_SmokeBomb.NS_SmokeBomb"));
    SSmoke = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_SmokeBomb.SC_SmokeBomb"));
    NSRally = LoadObject<UNiagaraSystem>(nullptr, TEXT("/Game/VFX/NS_RallyWave.NS_RallyWave"));
    SMega = LoadObject<USoundBase>(nullptr, TEXT("/Game/Audio/SC_Megafono.SC_Megafono"));
}

void AResistanceTool::UseTool(FVector TargetLocation)
{
    UWorld* World = GetWorld();
    if (!World) return;

    switch (ToolType)
    {
    case EToolType::Slingshot:
    {
        if (NSSlingshot) UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, NSSlingshot, TargetLocation);
        if (SSlingshot) UGameplayStatics::PlaySoundAtLocation(World, SSlingshot, TargetLocation);

        TArray<FOverlapResult> Overlaps;
        FCollisionShape Sphere = FCollisionShape::MakeSphere(EffectRadius);
        if (World->OverlapMultiByChannel(Overlaps, TargetLocation, FQuat::Identity, ECC_Visibility, Sphere))
        {
            for (const FOverlapResult& Hit : Overlaps)
            {
                if (AActor* HitActor = Hit.GetActor())
                {
                    if (IDamageable* Damageable = Cast<IDamageable>(HitActor))
                    {
                        Damageable->RecibirDano(50, TargetLocation, ETipoDano::Impacto);
                    }
                }
            }
        }
        break;
    }
    case EToolType::SmokeBomb:
    {
        if (NSSmoke) UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, NSSmoke, TargetLocation, FRotator::ZeroRotator, FVector(2.f));
        if (SSmoke) UGameplayStatics::PlaySoundAtLocation(World, SSmoke, TargetLocation);

        if (UAlsasuaCrowdSentiment* Sentiment = World->GetSubsystem<UAlsasuaCrowdSentiment>())
        {
            Sentiment->TriggerSocialEvent(TargetLocation, -20.f, EffectRadius);
        }

        DrawDebugSphere(World, TargetLocation, EffectRadius, 16, FColor::Green, false, 5.0f);
        break;
    }
    case EToolType::Megaphone:
    {
        if (NSRally) UNiagaraFunctionLibrary::SpawnSystemAtLocation(World, NSRally, TargetLocation, FRotator::ZeroRotator, FVector(1.5f));
        if (SMega) UGameplayStatics::PlaySoundAtLocation(World, SMega, TargetLocation);

        if (UAlsasuaCrowdSentiment* Sentiment = World->GetSubsystem<UAlsasuaCrowdSentiment>())
        {
            Sentiment->PopularSupport = FMath::Min(Sentiment->PopularSupport + 8.f, 100.f);
            Sentiment->TriggerSocialEvent(TargetLocation, 15.f, EffectRadius);
        }

        DrawDebugSphere(World, TargetLocation, EffectRadius, 16, FColor::Yellow, false, 5.0f);
        break;
    }
    }
}
