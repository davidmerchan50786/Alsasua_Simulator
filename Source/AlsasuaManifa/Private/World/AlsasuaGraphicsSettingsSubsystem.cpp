#include "World/AlsasuaGraphicsSettingsSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"

void UAlsasuaGraphicsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Registrar comando de consola
    IConsoleManager::Get().RegisterConsoleCommand(
        TEXT("alsasua.SetGraphicsProfile"),
        TEXT("Cambia el perfil gráfico: 0=Low, 1=Med, 2=High, 3=Ultra"),
        FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&UAlsasuaGraphicsSettingsSubsystem::ConsoleSetProfile),
        ECVF_Cheat
    );
}

void UAlsasuaGraphicsSettingsSubsystem::ApplyGraphicsProfile(EAlsasuaGraphicsProfile Profile)
{
    int32 Level = (int32)Profile;
    SetLumenQuality(Level);
    SetNaniteBudget(Level);

    // PostProcess AA Quality
    UKismetSystemLibrary::ExecuteConsoleCommand(GetWorld(), FString::Printf(TEXT("r.PostProcessAAQuality %d"), Level + 3));

    UE_LOG(LogAlsasua, Warning, TEXT("Perfil Gráfico cambiado a: %d"), Level);
}

void UAlsasuaGraphicsSettingsSubsystem::ConsoleSetProfile(const TArray<FString>& Args, UWorld* InWorld)
{
    if (Args.Num() > 0 && InWorld)
    {
        int32 Level = FCString::Atoi(*Args[0]);
        if (auto* Sub = InWorld->GetSubsystem<UAlsasuaGraphicsSettingsSubsystem>())
        {
            Sub->ApplyGraphicsProfile((EAlsasuaGraphicsProfile)FMath::Clamp(Level, 0, 3));
        }
    }
}

void UAlsasuaGraphicsSettingsSubsystem::SetLumenQuality(int32 Level)
{
    IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.SceneDetail"))->Set(Level * 0.33f + 0.1f);
}

void UAlsasuaGraphicsSettingsSubsystem::SetNaniteBudget(int32 Level)
{
    // Ajustar presupuestos de clústers Nanite según el perfil
}
