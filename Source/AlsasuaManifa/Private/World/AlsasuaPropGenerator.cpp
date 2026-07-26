#include "World/AlsasuaPropGenerator.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Components/StaticMeshComponent.h"

void UAlsasuaPropGenerator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadAssets();
}

void UAlsasuaPropGenerator::LoadAssets()
{
    // Cargar de manera perezosa algunos meshes y materiales (rutas de ejemplo)
    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();
    BannerMeshes.Add(Cast<UStaticMesh>(Streamable.LoadSynchronous(FSoftObjectPath("/Game/Props/Banners/Banner_01.Banner_01"))));
    BannerMeshes.Add(Cast<UStaticMesh>(Streamable.LoadSynchronous(FSoftObjectPath("/Game/Props/Banners/Banner_02.Banner_02"))));

    BannerMaterials.Add(Cast<UMaterialInterface>(Streamable.LoadSynchronous(FSoftObjectPath("/Game/Props/Materials/BannerMat_01.BannerMat_01"))));
    BannerMaterials.Add(Cast<UMaterialInterface>(Streamable.LoadSynchronous(FSoftObjectPath("/Game/Props/Materials/BannerMat_02.BannerMat_02"))));
}

void UAlsasuaPropGenerator::AssignPropToActor(AActor* Actor)
{
    if (!Actor) return;

    UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(Actor);
    MeshComp->RegisterComponent();
    MeshComp->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

    int32 MeshIdx = FMath::RandRange(0, BannerMeshes.Num()-1);
    int32 MatIdx = FMath::RandRange(0, BannerMaterials.Num()-1);

    MeshComp->SetStaticMesh(BannerMeshes.IsValidIndex(MeshIdx) ? BannerMeshes[MeshIdx] : nullptr);
    MeshComp->SetMaterial(0, BannerMaterials.IsValidIndex(MatIdx) ? BannerMaterials[MatIdx] : nullptr);
}
