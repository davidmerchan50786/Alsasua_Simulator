#include "World/AlsasuaGraphicsSettingsSubsystem.h"
#include "HAL/IConsoleManager.h"
#include "Kismet/KismetSystemLibrary.h"
#include "Engine/World.h"
#include "Misc/ConfigCacheIni.h"

namespace
{
    // Una sola vez por proceso, no una por mundo. RegisterConsoleCommand da de
    // alta un nombre global: como esto es un UWorldSubsystem, su Initialize
    // corre en CADA mundo —el del editor, cada sesión de PIE, cada carga de
    // nivel— y volvía a registrar el mismo comando cada vez. El delegado es
    // estático y recibe su propio UWorld, así que la instancia no pinta nada
    // aquí: se registra al primer mundo y se queda.
    IConsoleCommand* ComandoPerfil = nullptr;

    // Perfil que se aplica al arrancar una partida, configurable en ini:
    //
    //   [/Script/AlsasuaManifa.AlsasuaGraphicsSettingsSubsystem]
    //   PerfilArranque=3
    //
    // Estaba clavado a Ultra en el código, y eso pisaba lo que dice Config/ en
    // todos los mundos. Config/ está versionado y afinado (RESUMEN_TECNICO.md);
    // un subsistema que le sobreescribe media docena de r.* al abrir el nivel
    // hace que dos arranques con el mismo ini midan cosas distintas.
    int32 LeerPerfilDeArranque()
    {
        int32 Perfil = (int32)EAlsasuaGraphicsProfile::Ultra;
        GConfig->GetInt(TEXT("/Script/AlsasuaManifa.AlsasuaGraphicsSettingsSubsystem"),
                        TEXT("PerfilArranque"), Perfil, GGameIni);
        return FMath::Clamp(Perfil, 0, 3);
    }
}

bool UAlsasuaGraphicsSettingsSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
    // Ni mundos de editor, ni previsualizaciones, ni cocción: esto toca CVars
    // de render globales al proceso, así que crearlo en el mundo del editor
    // significaba cambiarle la calidad al editor por abrir un nivel.
    if (const UWorld* W = Cast<UWorld>(Outer))
    {
        return W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE;
    }
    return false;
}

void UAlsasuaGraphicsSettingsSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    if (!ComandoPerfil)
    {
        ComandoPerfil = IConsoleManager::Get().RegisterConsoleCommand(
            TEXT("alsasua.SetGraphicsProfile"),
            TEXT("Cambia el perfil gráfico: 0=Low, 1=Med, 2=High, 3=Ultra"),
            FConsoleCommandWithWorldAndArgsDelegate::CreateStatic(&UAlsasuaGraphicsSettingsSubsystem::ConsoleSetProfile),
            ECVF_Cheat
        );
    }

    // Sólo en partida de verdad. En PIE se deja el editor como esté: quien
    // quiera probar un perfil tiene el comando de consola y el widget de
    // ajustes, que es quien de verdad llama a este subsistema.
    if (const UWorld* W = GetWorld(); W && W->WorldType == EWorldType::Game)
    {
        ApplyGraphicsProfile((EAlsasuaGraphicsProfile)LeerPerfilDeArranque());
    }
}

void UAlsasuaGraphicsSettingsSubsystem::ApplyGraphicsProfile(EAlsasuaGraphicsProfile Profile)
{
    // Es BlueprintCallable y el enum es uint8: un Blueprint puede pasar un 7 y
    // las tablas de abajo son de cuatro elementos. Se acota aquí, una vez.
    const int32 Level = FMath::Clamp((int32)Profile, 0, 3);
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
