#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaCore.h"
#include "Contratos/AlsasuaContratosUI.h"
#include "AlsasuaGraphicsSettingsSubsystem.generated.h"

UENUM(BlueprintType)
enum class EAlsasuaGraphicsProfile : uint8 { Low, Medium, High, Ultra };

UCLASS()
class GF_WORLD_API UAlsasuaGraphicsSettingsSubsystem : public UWorldSubsystem, public IAlsasuaAjustesGraficos
{
    GENERATED_BODY()

public:
    /** Sólo en mundos de partida (Game/PIE): esto toca CVars de render globales
     *  al proceso, y creado en el mundo del editor le cambiaba la calidad al
     *  editor por el mero hecho de abrir un nivel. */
    virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "AAA|Video")
    void ApplyGraphicsProfile(EAlsasuaGraphicsProfile Profile);

    // Comando de consola: alsasua.SetGraphicsProfile [0-3]
    static void ConsoleSetProfile(const TArray<FString>& Args, UWorld* InWorld);

    //~ IAlsasuaAjustesGraficos
    virtual void AplicarPerfilGrafico(int32 Perfil) override
    {
        ApplyGraphicsProfile((EAlsasuaGraphicsProfile)FMath::Clamp(Perfil, 0, 3));
    }

private:
    void SetLumenQuality(int32 Level);
    void SetNaniteBudget(int32 Level);
};
