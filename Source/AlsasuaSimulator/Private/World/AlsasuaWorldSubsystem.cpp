#include "World/AlsasuaWorldSubsystem.h"
#include "Materials/MaterialParameterCollection.h"
#include "Materials/MaterialParameterCollectionInstance.h"
#include "Materials/MaterialInterface.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Kismet/GameplayStatics.h"
#include "Landscape.h"
#include "LandscapeComponent.h"
#include "Materials/Material.h"

static UMaterialInterface* BuscarMaterialFallbackTerreno()
{
    if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_Terreno_Orto.M_Terreno_Orto")))
    {
        return Mat;
    }
    if (UMaterialInterface* Mat = LoadObject<UMaterialInterface>(nullptr, TEXT("/Game/Materiales/M_TerrenoAlsasua.M_TerrenoAlsasua")))
    {
        return Mat;
    }
    return nullptr;
}

/**
 * Rellena el material del Landscape SÓLO si no tiene ninguno.
 *
 * Antes hacía dos cosas de más, y las dos duelen:
 *
 * 1. Si el material del Landscape se llamaba M_TerrenoAlsasua, lo SUSTITUÍA.
 *    Ese material lo borra y regenera Tools/ImportSatellite.py a propósito para
 *    que recoja la ortofoto nueva, así que esto le peleaba el terreno al
 *    pipeline: cada vez que se abría el mundo se le quitaba el material bueno.
 *
 * 2. Si no encontraba fallback, asignaba UMaterial::GetDefaultMaterial. O sea
 *    que ante la duda dejaba el terreno del pueblo en gris de motor, encima
 *    machacando lo que hubiera. Justo lo que el proyecto evita en todas partes:
 *    si no hay material, no se toca y que siga el que trajera.
 *
 * Y llamaba a MarkPackageDirty() en Initialize, o sea en cada PIE: el editor
 * salía pidiendo guardar un nivel que nadie había tocado a mano.
 *
 * En -game esto no llega a hacer nada, porque el terreno jugable es
 * ATerrenoGenerado (malla procedural) y no hay ALandscape. Es en el editor
 * donde molestaba.
 */
static void RellenarMaterialTerrenoSiFalta(UWorld* World)
{
    if (!World) return;

    TArray<AActor*> LandscapeActors;
    UGameplayStatics::GetAllActorsOfClass(World, ALandscape::StaticClass(), LandscapeActors);
    if (LandscapeActors.Num() == 0) return;

    UMaterialInterface* MaterialFallback = BuscarMaterialFallbackTerreno();
    if (!MaterialFallback)
    {
        UE_LOG(LogTemp, Log,
            TEXT("[Simulator] Landscape sin material y sin fallback generado; se deja como está."));
        return;
    }

    int32 Rellenados = 0;
    for (AActor* Actor : LandscapeActors)
    {
        ALandscape* LandscapeActor = Cast<ALandscape>(Actor);
        if (!LandscapeActor || LandscapeActor->LandscapeMaterial != nullptr) continue;

        LandscapeActor->LandscapeMaterial = MaterialFallback;
        ++Rellenados;
    }

    if (Rellenados > 0)
    {
        UE_LOG(LogTemp, Log, TEXT("[Simulator] %d Landscape sin material, rellenados con %s."),
            Rellenados, *MaterialFallback->GetName());
    }
}

void UAlsasuaWorldSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    if (UWorld* World = GetWorld())
    {
        RellenarMaterialTerrenoSiFalta(World);
    }
}

void UAlsasuaWorldSubsystem::SetGlobalWetness(float Wetness) {
    UMaterialParameterCollection* MPC = LoadObject<UMaterialParameterCollection>(
        nullptr, TEXT("/Game/Materials/MPC_AlsasuaGlobal.MPC_AlsasuaGlobal"));
    if (!MPC)
    {
        MPC = LoadObject<UMaterialParameterCollection>(
            nullptr, TEXT("/Game/Materiales/MPC_Clima.MPC_Clima"));
    }
    if (!MPC) return;

    UWorld* W = GetWorld();
    if (!W) return;

    UMaterialParameterCollectionInstance* Inst = W->GetParameterCollectionInstance(MPC);
    if (!Inst) return;

    const float ClampedWetness = FMath::Clamp(Wetness, 0.f, 1.f);

    // "Wetness" es el escalar que MPC_Clima tiene de verdad — lo crea
    // UCreadorMaterialEdificio junto con "Night" — y es el que leen los
    // materiales del pueblo. Escribir sólo GlobalWetness/RainIntensity/
    // PuddleOpacity, que es lo que había, no movía nada: esos tres nombres no
    // existen en esa colección, y poner un escalar que no está sale por un
    // warning y se queda ahí.
    Inst->SetScalarParameterValue(FName("Wetness"), ClampedWetness);

    // Los otros tres sí existen en MPC_AlsasuaGlobal, que es la colección que se
    // intenta primero. Si la cargada es ésa, esto la alimenta; si es MPC_Clima,
    // no encuentra los nombres y no pasa nada más allá del warning.
    Inst->SetScalarParameterValue(FName("GlobalWetness"), ClampedWetness);
    Inst->SetScalarParameterValue(FName("RainIntensity"), ClampedWetness * 0.8f);
    Inst->SetScalarParameterValue(FName("PuddleOpacity"), FMath::Clamp(ClampedWetness * 1.2f, 0.f, 1.f));
}
