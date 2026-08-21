// AlsasuaPropGenerator.h (capa MANIFA)
// Coloca pancartas sobre actores. Sigue sin tener llamantes — quién reparte
// pancartas y a qué actores es una decisión de gameplay, no de este fichero —,
// pero ya está listo para cuando alguien lo llame: sus rutas de ejemplo
// (/Game/Props/Banners/Banner_01…) no las crea nadie, así que si no están tira
// de AlsasuaMallaFab, que busca una pancarta entre lo bajado de Fab y cae a un
// plano del motor. Produce algo en vez de nada, y se sustituye solo en cuanto
// haya malla buena.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "AlsasuaPropGenerator.generated.h"

UCLASS()
class ALSASUAMANIFA_API UAlsasuaPropGenerator : public UWorldSubsystem
{
    GENERATED_BODY()

public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;

    UFUNCTION(BlueprintCallable, Category = "AAA|Props")
    void AssignPropToActor(AActor* Actor);

private:
    // Assets cargados con LoadSynchronous en Initialize y usados mucho después
    // en AssignPropToActor. Sin UPROPERTY nada sujeta esa carga: el GC puede
    // llevárselos entre una cosa y la otra.
    UPROPERTY() TArray<TObjectPtr<UStaticMesh>> BannerMeshes;
    UPROPERTY() TArray<TObjectPtr<UMaterialInterface>> BannerMaterials;

    void LoadAssets();
};
