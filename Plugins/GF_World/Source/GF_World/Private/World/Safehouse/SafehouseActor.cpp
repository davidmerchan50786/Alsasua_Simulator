#include "World/Safehouse/SafehouseActor.h"
#include "Components/BoxComponent.h"
#include "Politics/FactionSubsystem.h"
#include "Social/EvidenceSubsystem.h"


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

    if (UWorld* W = GetWorld())
    {
        if (UFactionSubsystem* FS = W->GetSubsystem<UFactionSubsystem>())
        {
            FFactionData Data = FS->GetFactionData(FName("ElCentro"));
            Data.Suspicion = FMath::Max(0.f, Data.Suspicion - 50.f);
            FS->RegisterFaction(Data);
        }
    }

    UE_LOG(LogTemp, Log, TEXT("Jugador a salvo en: %s. Sospecha reducida."), *SafehouseName);
}

void ASafehouseActor::ChangeDisguise(AActor* PlayerActor, FName NewOutfitId)
{
    if (!PlayerActor || !bIsUnlocked) return;

    UDisguiseComponent* Disguise = PlayerActor->FindComponentByClass<UDisguiseComponent>();
    if (!Disguise)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeDisguise: Jugador no tiene UDisguiseComponent."));
        return;
    }

    const EDisguiseType NewType = ParseOutfitName(NewOutfitId);
    if (NewType == EDisguiseType::None)
    {
        UE_LOG(LogTemp, Warning, TEXT("ChangeDisguise: Outfit desconocido: %s"), *NewOutfitId.ToString());
        return;
    }

    Disguise->EquipDisguise(NewType, false);
    UE_LOG(LogTemp, Log, TEXT("Disfraz cambiado a: %s"), *NewOutfitId.ToString());
}

void ASafehouseActor::DepositEvidence()
{
    if (UWorld* W = GetWorld())
    {
        if (UEvidenceSubsystem* ES = W->GetSubsystem<UEvidenceSubsystem>())
        {
            int32 Count = ES->CollectedEvidence.Num();
            if (Count > 0)
            {
                EvidenceDeposited.Add(Count);
                ES->CollectedEvidence.Empty();
                UE_LOG(LogTemp, Warning, TEXT("Depositadas %d pruebas en la caja fuerte. Evidencia asegurada."), Count);
            }
            else
            {
                UE_LOG(LogTemp, Log, TEXT("No hay evidencia para depositar."));
            }
        }
    }
}

EDisguiseType ASafehouseActor::ParseOutfitName(FName OutfitId)
{
    const FString Name = OutfitId.ToString().ToLower();

    if (Name.Contains(TEXT("momo")) || Name.Contains(TEXT("tradicional")) || Name.Contains(TEXT("sakoa")))
    {
        return EDisguiseType::Momotxorro;
    }
    if (Name.Contains(TEXT("casual")) || Name.Contains(TEXT("infiltrador")) || Name.Contains(TEXT("ropa")))
    {
        return EDisguiseType::Casual_Infiltrator;
    }
    if (Name.Contains(TEXT("press")) || Name.Contains(TEXT("prensa")) || Name.Contains(TEXT("periodista")))
    {
        return EDisguiseType::Press_Press;
    }

    return EDisguiseType::None;
}
