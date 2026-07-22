// ArmasComponent.h (capa GAMEPLAY)
// Armas del jugador: disparo (line trace) y melee, con daño vía IDamageable.
// Puerto de SistemaArmasExtendido (la parte de combate). Pon este componente
// en el personaje jugador.
#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "AlsasuaTypes.h"
#include "ArmasComponent.generated.h"

UCLASS(ClassGroup=(Alsasua), meta=(BlueprintSpawnableComponent))
class ALSASUAGAMEPLAY_API UArmasComponent : public UActorComponent
{
	GENERATED_BODY()

public:
	UArmasComponent();

	UPROPERTY(EditAnywhere, BlueprintReadWrite, Category="Armas") ETipoArma ArmaActual = ETipoArma::Punos;

	// munición por arma (índice = ETipoArma)
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category="Armas") TArray<int32> Municion;

	UFUNCTION(BlueprintCallable, Category="Armas") void CambiarArma(ETipoArma Arma) { ArmaActual = Arma; }
	UFUNCTION(BlueprintCallable, Category="Armas") void RecogerArma(ETipoArma Arma, int32 Cantidad);

	UFUNCTION(BlueprintCallable, Category="Armas") int32 MunicionActual() const { const int32 i = (int32)ArmaActual; return Municion.IsValidIndex(i) ? Municion[i] : 0; }
	UFUNCTION(BlueprintCallable, Category="Armas") FString NombreArma() const;
	UFUNCTION(BlueprintCallable, Category="Armas") bool EsCuerpoACuerpo() const { return ArmaActual == ETipoArma::Punos; }

	// Acción de "usar arma" (clic izquierdo). Decide melee o disparo según el arma.
	UFUNCTION(BlueprintCallable, Category="Armas") void UsarArma();

	virtual void TickComponent(float DeltaTime, ELevelTick TickType, FActorComponentTickFunction* ThisTickFunction) override;

protected:
	virtual void BeginPlay() override;

private:
	float Cooldown = 0.f;

	void GolpearMelee();
	void DispararFuego(int32 Dano, int32 Perdigones, float DispersionGrados, float Cadencia);

	bool ObtenerMira(FVector& OutOrigen, FVector& OutDir) const;
	float MultDispersionActual() const;
	void SubirBusqueda(int32 Cantidad) const;
	void Consecuencias(AActor* Victima) const;
};
