#include "Data/AlsasuaEntityDataAsset.h"
#include "Engine/AssetManager.h"
#include "Engine/StreamableManager.h"

void UAlsasuaEntityDataAsset::AsyncPreloadEntity(FString EntityID)
{
	if (!EntityLibrary.Contains(EntityID)) return;

	FEntityVisualProfile& Profile = EntityLibrary[EntityID];

	// Si el asset ya está cargado, no hacemos nada
	if (Profile.MeshAsset.IsValid()) return;

	// Usamos el StreamableManager global de Unreal para cargar en segundo plano
	FStreamableManager& Streamable = UAssetManager::GetStreamableManager();

	TSoftObjectPtr<USkeletalMesh> AssetPtr = Profile.MeshAsset;

	Streamable.RequestAsyncLoad(AssetPtr.ToSoftObjectPath(), FStreamableDelegate::CreateLambda([AssetPtr]()
	{
		if (AssetPtr.IsValid())
		{
			UE_LOG(LogTemp, Log, TEXT("Asset cargado asíncronamente: %s"), *AssetPtr.Get()->GetName());
		}
	}));
}
