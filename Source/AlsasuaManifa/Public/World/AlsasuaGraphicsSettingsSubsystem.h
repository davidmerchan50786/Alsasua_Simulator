#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaCore.h"
#include "AlsasuaGraphicsSettingsSubsystem.generated.h"

UENUM(BlueprintType)
enum class EAlsasuaGraphicsProfile : uint8 { Low, Medium, High, Ultra };

UCLASS()
class ALSASUAMANIFA_API UAlsasuaGraphicsSettingsSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "AAA|Video")
    void ApplyGraphicsProfile(EAlsasuaGraphicsProfile Profile);

    // Comando de consola: alsasua.SetGraphicsProfile [0-3]
    static void ConsoleSetProfile(const TArray<FString>& Args, UWorld* InWorld);

private:
    void SetLumenQuality(int32 Level);
    void SetNaniteBudget(int32 Level);
};
