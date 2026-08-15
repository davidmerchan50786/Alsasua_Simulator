#include "World/AlsasuaPropGenerator.h"
#include "Engine/StreamableManager.h"
#include "Engine/AssetManager.h"
#include "Components/StaticMeshComponent.h"
#include "World/AlsasuaMallaFab.h"
#include "Engine/StaticMesh.h"

void UAlsasuaPropGenerator::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);
    LoadAssets();
}

void UAlsasuaPropGenerator::LoadAssets()
{
    // Rutas de ejemplo que no existen en el proyecto (ver la nota de la cabecera).
    // Antes se añadía el resultado del Load a la lista pasara lo que pasara, así
    // que las listas se llenaban de nulls: AssignPropToActor sorteaba un índice
    // "válido", asignaba un mesh nulo y la pancarta salía invisible sin una sola
    // línea de log. Ahora sólo entra lo que ha cargado, y si no carga nada se
    // dice una vez.
    FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

    const TCHAR* RutasMalla[] = {
        TEXT("/Game/Props/Banners/Banner_01.Banner_01"),
        TEXT("/Game/Props/Banners/Banner_02.Banner_02"),
    };
    for (const TCHAR* R : RutasMalla)
    {
        if (UStaticMesh* M = Cast<UStaticMesh>(Streamable.LoadSynchronous(FSoftObjectPath(R))))
            BannerMeshes.Add(M);
    }

    // Si esas rutas de ejemplo no existen — que es lo normal, no las crea nadie —
    // se tira del resolvedor del proyecto, que busca una pancarta entre lo bajado
    // de Fab y cae a un plano del motor como último recurso. Así el sistema
    // produce algo en vez de nada: una pancarta plana con su material es mejor
    // punto de partida que ninguna, y se sustituye sola en cuanto haya malla.
    if (BannerMeshes.Num() == 0)
    {
        if (UStaticMesh* M = AlsasuaMallaFab::Resolver(
                TEXT("pancarta"), TEXT("/Engine/BasicShapes/Plane.Plane")))
        {
            BannerMeshes.Add(M);
        }
    }

    const TCHAR* RutasMat[] = {
        TEXT("/Game/Props/Materials/BannerMat_01.BannerMat_01"),
        TEXT("/Game/Props/Materials/BannerMat_02.BannerMat_02"),
    };
    for (const TCHAR* R : RutasMat)
    {
        if (UMaterialInterface* M = Cast<UMaterialInterface>(Streamable.LoadSynchronous(FSoftObjectPath(R))))
            BannerMaterials.Add(M);
    }

    if (BannerMeshes.Num() == 0)
    {
        UE_LOG(LogTemp, Warning,
            TEXT("AlsasuaPropGenerator: sin mallas de pancarta en /Game/Props/Banners; no hay pancartas."));
    }
}

void UAlsasuaPropGenerator::AssignPropToActor(AActor* Actor)
{
    if (!Actor) return;
    // Sin mallas no hay nada que colocar, y además RandRange(0, -1) con la lista
    // vacía no tiene sentido. Se sale antes de crear un componente huérfano.
    if (BannerMeshes.Num() == 0) return;

    UStaticMeshComponent* MeshComp = NewObject<UStaticMeshComponent>(Actor);
    MeshComp->RegisterComponent();
    MeshComp->AttachToComponent(Actor->GetRootComponent(), FAttachmentTransformRules::KeepRelativeTransform);

    int32 MeshIdx = FMath::RandRange(0, BannerMeshes.Num()-1);
    int32 MatIdx = FMath::RandRange(0, BannerMaterials.Num()-1);

    MeshComp->SetStaticMesh(BannerMeshes.IsValidIndex(MeshIdx) ? BannerMeshes[MeshIdx] : nullptr);
    MeshComp->SetMaterial(0, BannerMaterials.IsValidIndex(MatIdx) ? BannerMaterials[MatIdx] : nullptr);
}
