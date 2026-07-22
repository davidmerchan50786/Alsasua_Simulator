#include "World/Safehouse/SafehouseActor.h"
#include "Components/BoxComponent.h"
#include "Politics/FactionSubsystem.h"
#include "Social/EvidenceSubsystem.h"
#include "AlsasuaCharacter.h"

ASafehouseActor::ASafehouseActor()
{
    PrimaryActorTick.bCanEverTick = false;
    InteractionZone = CreateDefaultSubobject<UBoxComponent>(TEXT("InteractionZone"));
    RootComponent = InteractionZone;
    InteractionZone->SetBoxExtent(FVector(200.f, 200.f, 100.f));
}

void ASafehouseActor::BeginPlay()
{
    Super::BeginPlay();
}

void ASafehouseActor::EnterSafehouse(AActor* PlayerActor)
{
    if (!bIsUnlocked) return;

    // Al entrar a una casa segura, el nivel de sospecha baja drásticamente
    if (UWorld* W = GetWorld())
    {
        if (UFactionSubsystem* FS = W->GetSubsystem<UFactionSubsystem>())
        {
            // Reset de sospecha de El Centro sobre el jugador
            FFactionData Data = FS->GetFactionData(FName("ElCentro"));
            Data.Suspicion = FMath::Max(0.f, Data.Suspicion - 50.f);
            FS->RegisterFaction(Data);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Jugador a salvo en: %s. Sospecha reducida."), *SafehouseName);
}

void ASafehouseActor::ChangeDisguise(AActor* PlayerActor, FName NewOutfitId)
{
    // Lógica para cambiar el Mesh o Tags del personaje para confundir a la IA
    // Esto podría llamar a una función en AAlsasuaCharacter
    UE_LOG(LogTemp, Log, TEXT("Cambiando apariencia a: %s"), *NewOutfitId.ToString());
}

void ASafehouseActor::DepositEvidence()
{
    if (UWorld* W = GetWorld())
    {
        if (UEvidenceSubsystem* ES = W->GetSubsystem<UEvidenceSubsystem>())
        {
            int32 Count = ES->CollectedEvidence.Num();
            // Aquí se podría implementar una mecánica de guardado persistente de las pruebas
            UE_LOG(LogTemp, Warning, TEXT("Depositadas %d pruebas en la caja fuerte."), Count);
        }
    }
}
