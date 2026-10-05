#include "GuardianIluminacion.h"
#include "EngineUtils.h"
#include "Engine/World.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/ExponentialHeightFog.h"
#include "Engine/PostProcessVolume.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/SkyAtmosphereComponent.h"
#include "Components/VolumetricCloudComponent.h"
#include "HAL/IConsoleManager.h"

bool UGuardianIluminacion::ShouldCreateSubsystem(UObject* Outer) const
{
    // Sólo partida y PIE: en el mundo del editor destruiría actores del mapa que
    // se está editando, y se guardarían así.
    if (const UWorld* W = Cast<UWorld>(Outer))
    {
        return W->WorldType == EWorldType::Game || W->WorldType == EWorldType::PIE;
    }
    return false;
}

void UGuardianIluminacion::OnWorldBeginPlay(UWorld& InWorld)
{
    Super::OnWorldBeginPlay(InWorld);
    UnoDeCada(InWorld);
    SolJugable(InWorld);
    ExposicionSana(InWorld);
}

namespace
{
    // Deja uno solo de cada clase: el que se llame como el preferido si lo hay,
    // si no el primero. Devuelve cuántos ha quitado.
    template <typename T>
    int32 DejarUno(UWorld& W, const TCHAR* Preferido)
    {
        TArray<T*> Todos;
        for (TActorIterator<T> It(&W); It; ++It) Todos.Add(*It);
        if (Todos.Num() <= 1) return 0;

        // Mismo criterio que UAlsasuaAtmosphereController (nombre exacto primero),
        // para que con GF_Clima encendido los dos se queden con el mismo actor.
        T* Queda = nullptr;
        for (T* A : Todos) { if (A->GetFName() == FName(Preferido)) { Queda = A; break; } }
        if (!Queda) for (T* A : Todos) { if (A->GetName().StartsWith(Preferido)) { Queda = A; break; } }
        if (!Queda) Queda = Todos[0];
        int32 Quitados = 0;
        for (T* A : Todos)
        {
            if (A != Queda) { A->Destroy(); ++Quitados; }
        }
        return Quitados;
    }

    // Lo mismo para actores que se reconocen por su componente (SkyAtmosphere y
    // VolumetricCloud no tienen una clase de actor común en todos los mapas).
    template <typename TComp>
    int32 DejarUnoPorComponente(UWorld& W)
    {
        TArray<AActor*> Con;
        for (TActorIterator<AActor> It(&W); It; ++It)
        {
            if (It->FindComponentByClass<TComp>()) Con.Add(*It);
        }
        int32 Quitados = 0;
        for (int32 i = 1; i < Con.Num(); ++i) { Con[i]->Destroy(); ++Quitados; }
        return Quitados;
    }
}

void UGuardianIluminacion::UnoDeCada(UWorld& W)
{
    const int32 Soles   = DejarUno<ADirectionalLight>(W, TEXT("Atmosphere_Sun"));
    const int32 Cielos  = DejarUno<ASkyLight>(W, TEXT("Atmosphere_SkyLight"));
    const int32 Nieblas = DejarUno<AExponentialHeightFog>(W, TEXT("Atmosphere_Fog"));
    const int32 Atmos   = DejarUnoPorComponente<USkyAtmosphereComponent>(W);
    const int32 Nubes   = DejarUnoPorComponente<UVolumetricCloudComponent>(W);
    if (Soles + Cielos + Nieblas + Atmos + Nubes > 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("GuardianIluminacion: duplicados quitados -> soles %d, luces de cielo %d, nieblas %d, atmosferas %d, nubes %d."),
            Soles, Cielos, Nieblas, Atmos, Nubes);
    }
}

void UGuardianIluminacion::SolJugable(UWorld& W)
{
    ADirectionalLight* Sol = nullptr;
    for (TActorIterator<ADirectionalLight> It(&W); It; ++It) { Sol = *It; break; }

    if (!Sol)
    {
        Sol = W.SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(),
            FVector::ZeroVector, FRotator(-45.f, 30.f, 0.f));
        UE_LOG(LogTemp, Warning, TEXT("GuardianIluminacion: el mapa no traía sol; creado uno a -45."));
    }
    if (!Sol) return;

    UDirectionalLightComponent* Luz = Cast<UDirectionalLightComponent>(Sol->GetLightComponent());
    if (!Luz) return;

    // Una luz direccional ilumina hacia su vector forward: pitch negativo es sol
    // por encima del horizonte. Con pitch >= 0 alumbra desde abajo del terreno.
    const FRotator Rot = Sol->GetActorRotation();
    const bool bBajoHorizonte = Rot.Pitch > -5.f;
    const bool bApagado = Luz->Intensity <= 0.01f || !Luz->IsVisible();

    UE_LOG(LogTemp, Log, TEXT("GuardianIluminacion: sol %s pitch=%.1f intensidad=%.2f visible=%d."),
        *Sol->GetName(), Rot.Pitch, Luz->Intensity, Luz->IsVisible() ? 1 : 0);

    if (bBajoHorizonte || bApagado)
    {
        Luz->SetMobility(EComponentMobility::Movable);
        if (bBajoHorizonte)
        {
            Sol->SetActorRotation(FRotator(-45.f, Rot.Yaw, 0.f));
        }
        if (bApagado)
        {
            Luz->SetVisibility(true);
            Luz->SetIntensity(10.f);   // lux: el valor por defecto de una DirectionalLight
        }
        UE_LOG(LogTemp, Warning, TEXT("GuardianIluminacion: sol corregido (bajo horizonte=%d, apagado=%d)."),
            bBajoHorizonte ? 1 : 0, bApagado ? 1 : 0);
    }

    // La luz de cielo captura el cielo al cargar; si el sol ha cambiado, se recaptura.
    for (TActorIterator<ASkyLight> It(&W); It; ++It)
    {
        if (USkyLightComponent* SL = It->GetLightComponent())
        {
            SL->SetMobility(EComponentMobility::Movable);
            SL->RecaptureSky();
        }
    }
}

void UGuardianIluminacion::ExposicionSana(UWorld& W)
{
    for (TActorIterator<APostProcessVolume> It(&W); It; ++It)
    {
        FPostProcessSettings& S = It->Settings;
        UE_LOG(LogTemp, Log,
            TEXT("GuardianIluminacion: post-process %s unbound=%d metodo=%d(ov %d) bias=%.2f(ov %d) minEV=%.2f(ov %d) maxEV=%.2f(ov %d)."),
            *It->GetName(), It->bUnbound ? 1 : 0,
            (int32)S.AutoExposureMethod, S.bOverride_AutoExposureMethod ? 1 : 0,
            S.AutoExposureBias, S.bOverride_AutoExposureBias ? 1 : 0,
            S.AutoExposureMinBrightness, S.bOverride_AutoExposureMinBrightness ? 1 : 0,
            S.AutoExposureMaxBrightness, S.bOverride_AutoExposureMaxBrightness ? 1 : 0);

        bool bTocado = false;
        // Manual sin calibrar deja la imagen negra o quemada según la luz real.
        if (S.bOverride_AutoExposureMethod && S.AutoExposureMethod == EAutoExposureMethod::AEM_Manual)
        {
            S.AutoExposureMethod = EAutoExposureMethod::AEM_Histogram;
            bTocado = true;
        }
        // Una compensación fuera de +-4 EV es multiplicar o dividir la luz por 16.
        if (S.bOverride_AutoExposureBias && FMath::Abs(S.AutoExposureBias) > 4.f)
        {
            S.AutoExposureBias = 0.f;
            bTocado = true;
        }
        // Un rango de exposición invertido o clavado arriba tampoco deja ver nada.
        if (S.bOverride_AutoExposureMinBrightness && S.bOverride_AutoExposureMaxBrightness
            && S.AutoExposureMinBrightness > S.AutoExposureMaxBrightness)
        {
            S.bOverride_AutoExposureMinBrightness = false;
            S.bOverride_AutoExposureMaxBrightness = false;
            bTocado = true;
        }
        if (bTocado)
        {
            UE_LOG(LogTemp, Warning, TEXT("GuardianIluminacion: exposición de %s corregida."), *It->GetName());
        }
    }
}

// ─────────────────────────────────────────────────────────────────────────────
//  Consola, para usar en mitad de una partida negra:
//    Alsasua.Luz          vuelca al log el estado de toda la iluminación
//    Alsasua.Luz.Forzar   pone un mediodía a la fuerza (sol, cielo, exposición)
//  Sirve para separar «el arranque la dejó mal» de «algo la estropea después»:
//  si Forzar la arregla y al rato vuelve a negro, hay un sistema que la pisa.
// ─────────────────────────────────────────────────────────────────────────────
static void VolcarLuz(UWorld* W)
{
    if (!W) return;
    int32 N = 0;
    for (TActorIterator<ADirectionalLight> It(W); It; ++It, ++N)
    {
        const ULightComponent* L = It->GetLightComponent();
        UE_LOG(LogTemp, Warning, TEXT("Alsasua.Luz: sol %s pitch=%.1f yaw=%.1f intensidad=%.2f visible=%d"),
            *It->GetName(), It->GetActorRotation().Pitch, It->GetActorRotation().Yaw,
            L ? L->Intensity : -1.f, (L && L->IsVisible()) ? 1 : 0);
    }
    if (N == 0) UE_LOG(LogTemp, Warning, TEXT("Alsasua.Luz: NO hay ninguna luz direccional."));
    for (TActorIterator<ASkyLight> It(W); It; ++It)
    {
        const USkyLightComponent* SL = It->GetLightComponent();
        UE_LOG(LogTemp, Warning, TEXT("Alsasua.Luz: luz de cielo %s intensidad=%.2f visible=%d"),
            *It->GetName(), SL ? SL->Intensity : -1.f, (SL && SL->IsVisible()) ? 1 : 0);
    }
    for (TActorIterator<APostProcessVolume> It(W); It; ++It)
    {
        const FPostProcessSettings& S = It->Settings;
        UE_LOG(LogTemp, Warning, TEXT("Alsasua.Luz: post-process %s unbound=%d prioridad=%.1f peso=%.2f metodo=%d bias=%.2f(ov %d)"),
            *It->GetName(), It->bUnbound ? 1 : 0, It->Priority, It->BlendWeight,
            (int32)S.AutoExposureMethod, S.AutoExposureBias, S.bOverride_AutoExposureBias ? 1 : 0);
    }
}

static FAutoConsoleCommandWithWorld GCmdLuz(
    TEXT("Alsasua.Luz"),
    TEXT("Vuelca al log el estado de soles, luces de cielo y post-process."),
    FConsoleCommandWithWorldDelegate::CreateStatic(&VolcarLuz));

static FAutoConsoleCommandWithWorld GCmdLuzForzar(
    TEXT("Alsasua.Luz.Forzar"),
    TEXT("Pone un mediodía a la fuerza: un sol a -45 y 10 lux, cielo recapturado, exposición automática."),
    FConsoleCommandWithWorldDelegate::CreateLambda([](UWorld* W)
    {
        if (!W) return;
        ADirectionalLight* Sol = nullptr;
        for (TActorIterator<ADirectionalLight> It(W); It; ++It) { Sol = *It; break; }
        if (!Sol) Sol = W->SpawnActor<ADirectionalLight>(ADirectionalLight::StaticClass(), FVector::ZeroVector, FRotator(-45.f, 30.f, 0.f));
        if (Sol)
        {
            if (ULightComponent* L = Sol->GetLightComponent())
            {
                L->SetMobility(EComponentMobility::Movable);
                L->SetVisibility(true);
                L->SetIntensity(FMath::Max(L->Intensity, 10.f));
            }
            Sol->SetActorRotation(FRotator(-45.f, Sol->GetActorRotation().Yaw, 0.f));
        }
        for (TActorIterator<ASkyLight> It(W); It; ++It)
        {
            if (USkyLightComponent* SL = It->GetLightComponent())
            {
                SL->SetMobility(EComponentMobility::Movable);
                SL->SetVisibility(true);
                SL->SetIntensity(FMath::Max(SL->Intensity, 1.f));
                SL->RecaptureSky();
            }
        }
        for (TActorIterator<APostProcessVolume> It(W); It; ++It)
        {
            FPostProcessSettings& S = It->Settings;
            S.bOverride_AutoExposureMethod = false;
            S.bOverride_AutoExposureBias = false;
            S.bOverride_AutoExposureMinBrightness = false;
            S.bOverride_AutoExposureMaxBrightness = false;
        }
        UE_LOG(LogTemp, Warning, TEXT("Alsasua.Luz.Forzar: mediodía aplicado; estado tras forzar:"));
        VolcarLuz(W);
    }));
