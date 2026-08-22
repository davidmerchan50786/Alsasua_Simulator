#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaBannerRenderer.generated.h"

/** Sistema que renderiza texto dinámico en las mallas de las pancartas */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_UI_API UAlsasuaBannerRenderer : public UActorComponent
{
    GENERATED_BODY()

public:
    UAlsasuaBannerRenderer();

    // Renderiza un mensaje en el componente de malla especificado
    UFUNCTION(BlueprintCallable, Category = "AAA|Visuals")
    void RenderMessageOnBanner(UStaticMeshComponent* TargetMesh, FString Message);

    // Lista de mensajes posibles según el contexto social
    UPROPERTY(EditAnywhere, Category = "AAA|Localization")
    TArray<FString> MessagePool;

protected:
    UPROPERTY(Transient)
    class UCanvasRenderTarget2D* BannerCanvas;

    UPROPERTY(Transient)
    class UMaterialInstanceDynamic* DynamicMaterial;
};
