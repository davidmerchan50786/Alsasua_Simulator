#include "Audio/AlsasuaWhisperManager.h"
#include "Kismet/GameplayStatics.h"
UAlsasuaWhisperManager::UAlsasuaWhisperManager() { PrimaryComponentTick.bCanEverTick = false; }
void UAlsasuaWhisperManager::PlaySpatialWhisper(USoundBase* WhisperSound, float Intensity) {
    if (!WhisperSound) return;
    FVector RandomLoc = GetComponentLocation() + (FMath::VRand() * 150.0f);
    UGameplayStatics::PlaySoundAtLocation(this, WhisperSound, RandomLoc, Intensity);
}
