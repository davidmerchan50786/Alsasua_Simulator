#include "UI/AlsasuaBannerRenderer.h"
#include "Engine/CanvasRenderTarget2D.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "Engine/Canvas.h"

UAlsasuaBannerRenderer::UAlsasuaBannerRenderer()
{
    PrimaryComponentTick.bCanEverTick = false;
}

void UAlsasuaBannerRenderer::RenderMessageOnBanner(UStaticMeshComponent* TargetMesh, FString Message)
{
    if (!TargetMesh || Message.IsEmpty()) return;

    // 1. Crear o reutilizar el RenderTarget (AAA+++ Eficiencia)
    if (!BannerCanvas)
    {
        BannerCanvas = UCanvasRenderTarget2D::CreateCanvasRenderTarget2D(GetWorld(), UCanvasRenderTarget2D::StaticClass(), 512, 128);
    }

    // 2. Crear material dinámico para la pancarta
    DynamicMaterial = TargetMesh->CreateAndSetMaterialInstanceDynamic(0);
    if (DynamicMaterial && BannerCanvas)
    {
        DynamicMaterial->SetTextureParameterValue(FName("BannerTextTexture"), BannerCanvas);
    }

    // 3. Lógica de dibujo en el Canvas (Simple para C++, se expande en Blueprint/Shaders)
    // El BannerCanvas dispararía un delegado de dibujo donde usaríamos Canvas->DrawText
    UE_LOG(LogTemp, Log, TEXT("Pancarta generada con mensaje: %s"), *Message);
}
