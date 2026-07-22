// PisoFrancoActor.h (capa GAMEPLAY)
// Refugio: al entrar el jugador cura, baja el nivel de búsqueda y fija el
// punto de reaparición. Puerto de PisoFranco.cs + flujo de respawn.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "PisoFrancoActor.generated.h"

class UBoxComponent;

UCLASS()
class ALSASUAGAMEPLAY_API APisoFrancoActor : public AActor
{
	GENERATED_BODY()

public:
	APisoFrancoActor();

	UPROPERTY(EditAnywhere, Category="PisoFranco") FString Nombre = TEXT("Piso franco");
	UPROPERTY(EditAnywhere, Category="PisoFranco") float CuracionPorSeg = 8.f;
	UPROPERTY(EditAnywhere, Category="PisoFranco") float CalorPorSeg = 0.5f;   // estrellas/seg que baja

protected:
	virtual void Tick(float DeltaTime) override;

	UPROPERTY(VisibleAnywhere) UBoxComponent* Zona;

	UFUNCTION() void OnEntra(UPrimitiveComponent* Comp, AActor* Otro, UPrimitiveComponent* OtroComp, int32 Idx, bool bDesde, const FHitResult& Hit);
	UFUNCTION() void OnSale(UPrimitiveComponent* Comp, AActor* Otro, UPrimitiveComponent* OtroComp, int32 Idx);

private:
	bool bJugadorDentro = false;
	float AcumBajada = 0.f;   // acumula calor fraccionario hasta restar 1 estrella
	void FijarRespawn(AActor* Jugador);
};
