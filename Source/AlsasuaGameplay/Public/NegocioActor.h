// NegocioActor.h (capa GAMEPLAY)
// Bar / comercio / empresa / industria extorsionable. Puerto de Negocio (Unity).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaTypes.h"
#include "NegocioActor.generated.h"

UENUM(BlueprintType)
enum class EEstadoNegocio : uint8 { Libre, Extorsionado };

UCLASS()
class ALSASUAGAMEPLAY_API ANegocioActor : public AActor
{
	GENERATED_BODY()

public:
	ANegocioActor();

	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Negocio") ETipoNegocio Tipo = ETipoNegocio::Bar;
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Negocio") FString Nombre = TEXT("Negocio");
	UPROPERTY(BlueprintReadOnly, Category="Negocio") EEstadoNegocio Estado = EEstadoNegocio::Libre;

	UFUNCTION(BlueprintPure, Category="Negocio") int32 IngresoMin() const;
	UFUNCTION(BlueprintCallable, Category="Negocio") void PonerBajoControl() { Estado = EEstadoNegocio::Extorsionado; }
	UFUNCTION(BlueprintCallable, Category="Negocio") void Liberar()          { Estado = EEstadoNegocio::Libre; }

	// Llamar desde el sistema de interacción (tecla de uso).
	UFUNCTION(BlueprintCallable, Category="Negocio") void Interactuar();

protected:
	virtual void BeginPlay() override;
	virtual void EndPlay(const EEndPlayReason::Type Reason) override;
};
