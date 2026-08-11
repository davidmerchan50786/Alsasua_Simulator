// AlsasuaPropGenerator.h (capa MANIFA)
// Coloca pancartas sobre actores. DORMIDO: no lo llama nadie, y sus rutas
// (/Game/Props/Banners/Banner_01, /Game/Props/Materials/BannerMat_01) son de
// ejemplo — no las genera ningún CreadorMalla* ni vienen en ningún pack del
// asset_manifest, así que no existen en el proyecto.
//
// Si algún día se despierta: o se crean esos assets, o se le enchufa
// AlsasuaMallaFab como hace el resto del mundo, que resuelve por palabra clave
// y degrada solo. Mientras tanto LoadAssets() ya no mete nulls en las listas,
// para que el fallo se vea en el log en vez de dejar pancartas invisibles.
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
    TArray<UStaticMesh*> BannerMeshes;
    TArray<UMaterialInterface*> BannerMaterials;

    void LoadAssets();
};
