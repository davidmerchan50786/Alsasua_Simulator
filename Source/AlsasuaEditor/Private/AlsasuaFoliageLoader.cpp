#include "AlsasuaFoliageLoader.h"
#include "EditorAssetLibrary.h"
#include "Engine/StaticMesh.h"
#include "FoliageType.h"
#include "FoliageType_InstancedStaticMesh.h"
#include "FoliageInstancedStaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/Engine.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "AssetRegistry/AssetRegistryModule.h"

static const TArray<TPair<FString, FString>> SpecieToSourcePath = {
	{TEXT("Tilia"),       TEXT("/Game/Megascans/SM_Tilia_Foliage")},
	{TEXT("Platanus"),    TEXT("/Game/Megascans/SM_Platanus_Foliage")},
	{TEXT("QuercusRobur"),TEXT("/Game/Megascans/SM_Quercus_Foliage")},
	{TEXT("Pinus"),       TEXT("/Game/Megascans/SM_Pinus_Foliage")},
	{TEXT("Fagus"),       TEXT("/Game/Megascans/SM_Fagus_Foliage")},
	{TEXT("Betula"),      TEXT("/Game/Megascans/SM_Betula_Foliage")},
	{TEXT("Populus"),     TEXT("/Game/Megascans/SM_Populus_Foliage")},
	{TEXT("Salix"),       TEXT("/Game/Megascans/SM_Salix_Foliage")},
	{TEXT("Prunus"),      TEXT("/Game/Megascans/SM_Prunus_Foliage")},
	{TEXT("Acer"),        TEXT("/Game/Megascans/SM_Acer_Foliage")},
};

bool UAlsasuaFoliageLoader::ScanAndRegisterFoliage()
{
	int32 Found = 0;
	int32 Missing = 0;

	for (const auto& Pair : SpecieToSourcePath)
	{
		if (UEditorAssetLibrary::DoesAssetExist(Pair.Value))
		{
			UE_LOG(LogTemp, Log, TEXT("[Foliage] %s found at %s"), *Pair.Key, *Pair.Value);
			++Found;
		}
		else
		{
			UE_LOG(LogTemp, Warning, TEXT("[Foliage] %s NOT found at %s — import from Fab Megascans first"), *Pair.Key, *Pair.Value);
			++Missing;
		}
	}

	UE_LOG(LogTemp, Log, TEXT("[Foliage] Scan complete: %d found, %d missing (import via Fab plugin)"), Found, Missing);
	return Missing == 0;
}

bool UAlsasuaFoliageLoader::ReplaceProceduralTreesWithFoliage()
{
	// Check if any Megascans foliage is available
	bool bAnyFoliage = false;
	for (const auto& Pair : SpecieToSourcePath)
	{
		if (UEditorAssetLibrary::DoesAssetExist(Pair.Value))
		{
			bAnyFoliage = true;
			break;
		}
	}

	if (!bAnyFoliage)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Foliage] No Megascans foliage found. Import tree packs from Fab first."));
		UE_LOG(LogTemp, Log, TEXT("[Foliage] Workflow: Fab.com → Quixel Megascans → European Deciduous → Download"));
		UE_LOG(LogTemp, Log, TEXT("[Foliage] Place .uasset files at paths like /Game/Megascans/SM_Tilia_Foliage"));
		return false;
	}

	UE_LOG(LogTemp, Log, TEXT("[Foliage] FoliageType creation needs UE5 foliage wizard — use editor Foliage tool directly"));
	UE_LOG(LogTemp, Log, TEXT("[Foliage] Place Megascans meshes at /Game/Megascans/SM_*_Foliage, then use Foliage paint"));
	return false;
}
