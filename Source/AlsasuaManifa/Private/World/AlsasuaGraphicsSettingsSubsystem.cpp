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

    // Fidelidad gráfica 100% con realidad: Ultra por defecto.
    ApplyGraphicsProfile(EAlsasuaGraphicsProfile::Ultra);
}

void UAlsasuaGraphicsSettingsSubsystem::ApplyGraphicsProfile(EAlsasuaGraphicsProfile Profile)
{
    int32 Level = (int32)Profile;
    SetLumenQuality(Level);
    SetNaniteBudget(Level);

    UWorld* W = GetWorld();

    // PostProcess AA Quality
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.PostProcessAAQuality %d"), Level + 3));

    // Sombras
    static const int32 ShadowCascades[] = {2, 3, 4, 6};
    static const int32 ShadowRes[]      = {512, 1024, 2048, 4096};
    static const int32 VSMPages[]       = {512, 1024, 2048, 4096};
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.Shadow.CSM.MaxCascades %d"),          ShadowCascades[Level]));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.Shadow.MaxResolution %d"),            ShadowRes[Level]));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.Shadow.Virtual.MaxPhysicalPages %d"), VSMPages[Level]));

    // AO (SSAO como complemento a Lumen)
    static const int32 AOLevels[]  = {0, 1, 2, 3};
    static const int32 AOQuality[] = {0, 25, 75, 100};
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.AmbientOcclusionLevels %d"),   AOLevels[Level]));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.AmbientOcclusionMaxQuality %d"), AOQuality[Level]));

    // Post-process — bloom, motion blur, DoF, tonemapper
    static const int32 BloomQ[]   = {1, 2, 4, 5};
    static const int32 MBQ[]      = {0, 1, 3, 4};
    static const int32 DofQ[]     = {0, 1, 2, 4};
    static const int32 TonemapQ[] = {0, 2, 4, 5};
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.BloomQuality %d"),       BloomQ[Level]));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.MotionBlurQuality %d"),  MBQ[Level]));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.DepthOfFieldQuality %d"), DofQ[Level]));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.Tonemapper.Quality %d"), TonemapQ[Level]));

    // Texturas
    static const int32 MaxAniso[] = {4, 8, 12, 16};
    static const int32 TexSize[]  = {512, 1024, 2048, 4096};
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.MaxAnisotropy %d"),                    MaxAniso[Level]));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.Streaming.MaxEffectiveTextureSize %d"), TexSize[Level]));

    // Nubes volumétricas (sólo High y Ultra)
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.VolumetricCloud %d"),              Level >= 2 ? 1 : 0));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.VolumetricCloud.ShadowMap.Enable %d"), Level >= 3 ? 1 : 0));

    // Distance Fields
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.DistanceFieldShadowing %d"), Level >= 1 ? 1 : 0));
    UKismetSystemLibrary::ExecuteConsoleCommand(W, FString::Printf(TEXT("r.DistanceFieldAO %d"),        Level >= 2 ? 1 : 0));

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
    if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Lumen.SceneDetail")))
    {
        CVar->Set(Level * 0.33f + 0.1f);
    }
}

void UAlsasuaGraphicsSettingsSubsystem::SetNaniteBudget(int32 Level)
{
    if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite.MaxPixelsPerEdge")))
    {
        float Values[] = {2.0f, 1.5f, 1.0f, 0.5f};
        CVar->Set(Values[FMath::Clamp(Level, 0, 3)]);
    }
    if (IConsoleVariable* CVar = IConsoleManager::Get().FindConsoleVariable(TEXT("r.Nanite.ProxyTriangleThreshold")))
    {
        int32 Values[] = {1000000, 500000, 200000, 50000};
        CVar->Set(Values[FMath::Clamp(Level, 0, 3)]);
    }
}
