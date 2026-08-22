#pragma once

#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "AlsasuaEntityDataAsset.generated.h"

/** Estructura para definir un tipo de NPC o Vehículo sin cargarlo en memoria */
USTRUCT(BlueprintType)
struct FEntityVisualProfile
{
	GENERATED_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	FString EntityName;

	// Referencia "suave": Solo es una ruta de archivo, no ocupa RAM hasta que se pide.
	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	TSoftObjectPtr<USkeletalMesh> MeshAsset;

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Assets")
	TSoftClassPtr<UAnimInstance> AnimationClass;
};

UCLASS()
class GF_CORE_API UAlsasuaEntityDataAsset : public UPrimaryDataAsset
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category = "Library")
	TMap<FString, FEntityVisualProfile> EntityLibrary;

	// Función para precargar un asset de forma asíncrona
	UFUNCTION(BlueprintCallable, Category = "Optimization")
	void AsyncPreloadEntity(FString EntityID);
};
