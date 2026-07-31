// CargadorPOI.h (capa WORLD)
// Carga POIs desde poi_data.json y los coloca en el mundo como actores
// con componentes de interacción, etiquetas de gameplay y texto 3D.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "CargadorPOI.generated.h"

USTRUCT(BlueprintType)
struct FPOIData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadOnly) FString Id;
	UPROPERTY(BlueprintReadOnly) FString Nombre;
	UPROPERTY(BlueprintReadOnly) FString Tipo;
	UPROPERTY(BlueprintReadOnly) FString Subtipo;
	UPROPERTY(BlueprintReadOnly) FString Calle;
	UPROPERTY(BlueprintReadOnly) FString Descripcion;
	UPROPERTY(BlueprintReadOnly) bool bInteractuable = false;
	UPROPERTY(BlueprintReadOnly) bool bExtorsionable = false;
	UPROPERTY(BlueprintReadOnly) FString Faccion;
	UPROPERTY(BlueprintReadOnly) FString Dialogo;
	UPROPERTY(BlueprintReadOnly) FVector PosicionMundo = FVector::ZeroVector;
};

UCLASS()
class ALSASUAWORLD_API UCargadorPOI : public UWorldSubsystem
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, Category="Mundo") FString RutaRelativa = TEXT("Datos/poi_data.json");
	UPROPERTY(EditAnywhere, Category="Mundo") bool bAutoCargar = true;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	UFUNCTION(BlueprintCallable, Category="Mundo")
	int32 Cargar();

	// Consulta POIs por tipo.
	UFUNCTION(BlueprintCallable, Category="Mundo")
	TArray<FPOIData> GetPOIsByTipo(const FString& Tipo) const;

	UFUNCTION(BlueprintCallable, Category="Mundo")
	FPOIData GetPOIById(const FString& Id) const;

	UPROPERTY(BlueprintReadOnly, Category="Mundo")
	TArray<FPOIData> TodosLosPOIs;

private:
	void ColocarPOI(const FPOIData& Data);
};
