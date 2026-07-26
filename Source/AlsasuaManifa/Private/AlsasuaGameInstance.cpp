#include "AlsasuaGameInstance.h"
#include "AlsasuaCore.h"
#include "Kismet/GameplayStatics.h"
#include "Core/AlsasuaSaveGame.h"
#include "AlsasuaCharacter.h"
#include "GAS/AlsasuaAbilitySystemComponent.h"
#include "AlsasuaAttributeSet.h"

UAlsasuaGameInstance::UAlsasuaGameInstance()
{
}

void UAlsasuaGameInstance::SaveGame(FString SlotName)
{
    UAlsasuaSaveGame* SaveInstance = Cast<UAlsasuaSaveGame>(UGameplayStatics::CreateSaveGameObject(UAlsasuaSaveGame::StaticClass()));
    if (!SaveInstance) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (PC && PC->GetPawn())
    {
        AActor* Pawn = PC->GetPawn();
        SaveInstance->PlayerLocation = Pawn->GetActorLocation();
        SaveInstance->PlayerRotation = Pawn->GetActorRotation();

        UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
        if (ASC)
        {
            UAlsasuaAttributeSet* AS = ASC->GetSet<UAlsasuaAttributeSet>();
            if (AS)
            {
                SaveInstance->SavedPopularSupport = AS->GetPopularSupport();
                SaveInstance->SavedWantedLevel = AS->GetWantedLevel();
            }
        }
    }

    SaveInstance->SaveSlotName = SlotName;
    UGameplayStatics::SaveGameToSlot(SaveInstance, SlotName, 0);
    UE_LOG(LogAlsasua, Log, TEXT("Game saved to slot: %s"), *SlotName);
}

void UAlsasuaGameInstance::LoadGame(FString SlotName)
{
    if (!UGameplayStatics::DoesSaveGameExist(SlotName, 0)) return;

    UAlsasuaSaveGame* Loaded = Cast<UAlsasuaSaveGame>(
        UGameplayStatics::LoadGameFromSlot(SlotName, 0));
    if (!Loaded) return;

    UWorld* World = GetWorld();
    if (!World) return;

    APlayerController* PC = UGameplayStatics::GetPlayerController(World, 0);
    if (PC && PC->GetPawn())
    {
        AActor* Pawn = PC->GetPawn();
        Pawn->SetActorLocation(Loaded->PlayerLocation);
        Pawn->SetActorRotation(Loaded->PlayerRotation);

        UAbilitySystemComponent* ASC = UAbilitySystemGlobals::GetAbilitySystemComponentFromActor(Pawn);
        if (ASC)
        {
            UAlsasuaAttributeSet* AS = ASC->GetSet<UAlsasuaAttributeSet>();
            if (AS)
            {
                ASC->ApplyModToAttribute(AS->GetPopularSupportAttribute(), EGameplayModOp::Override, Loaded->SavedPopularSupport);
                ASC->ApplyModToAttribute(AS->GetWantedLevelAttribute(), EGameplayModOp::Override, Loaded->SavedWantedLevel);
            }
        }
    }

    UE_LOG(LogAlsasua, Log, TEXT("Game loaded from slot: %s"), *SlotName);
}

bool UAlsasuaGameInstance::HasSaveData(FString SlotName) const
{
    return UGameplayStatics::DoesSaveGameExist(SlotName, 0);
}
