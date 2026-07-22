// ManifestacionSubsystem.h (capa GAMEPLAY)
// Orquesta una manifestación: concentración -> marcha -> dispersión. El tamaño
// escala con el apoyo popular; mientras es pacífica sube el apoyo; una carga
// policial la disuelve y sube la búsqueda. Puerto de SistemaManifestacion.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Tickable.h"
#include "ManifestacionSubsystem.generated.h"

class AManifestanteActor;

UENUM(BlueprintType)
enum class EEstadoManifestacion : uint8 { Inactiva, Concentracion, Marcha, Dispersando };

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnManifestacionEstado, EEstadoManifestacion, Estado);

UCLASS()
class ALSASUAGAMEPLAY_API UManifestacionSubsystem : public UGameInstanceSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	UPROPERTY(BlueprintAssignable) FOnManifestacionEstado OnEstado;

	UPROPERTY(EditAnywhere, Category="Manifestacion") int32 TamMin = 10;
	UPROPERTY(EditAnywhere, Category="Manifestacion") int32 TamMax = 60;
	UPROPERTY(EditAnywhere, Category="Manifestacion") float RadioConcentracion = 1200.f;  // 12 m
	UPROPERTY(EditAnywhere, Category="Manifestacion") float TasaApoyoPorSeg = 0.4f;
	UPROPERTY(EditAnywhere, Category="Manifestacion") float DuracionConcentracion = 20.f;
	UPROPERTY(EditAnywhere, Category="Manifestacion") float VelocidadMarcha = 250.f;       // cm/s
	UPROPERTY(EditAnywhere, Category="Manifestacion") float RadioPresionPolicia = 1800.f;
	UPROPERTY(EditAnywhere, Category="Manifestacion") int32 PoliciasParaCarga = 3;
	UPROPERTY(EditAnywhere, Category="Manifestacion") float DuracionDispersion = 12.f;

	// Convoca una manifestación en un punto, con ruta de marcha opcional.
	UFUNCTION(BlueprintCallable, Category="Manifestacion")
	bool Convocar(FVector Punto, const TArray<FVector>& RutaMarcha);

	UFUNCTION(BlueprintCallable, Category="Manifestacion") void Disolver(bool bPorPolicia);
	UFUNCTION(BlueprintCallable, Category="Manifestacion") bool Activa() const { return Estado != EEstadoManifestacion::Inactiva; }
	UFUNCTION(BlueprintCallable, Category="Manifestacion") EEstadoManifestacion EstadoActual() const { return Estado; }
	UFUNCTION(BlueprintCallable, Category="Manifestacion") int32 NumManifestantes() const { return Multitud.Num(); }

	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UManifestacionSubsystem, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }

private:
	EEstadoManifestacion Estado = EEstadoManifestacion::Inactiva;
	UPROPERTY() TArray<AManifestanteActor*> Multitud;
	FVector PuntoActual = FVector::ZeroVector;
	TArray<FVector> Ruta;
	int32 RutaIdx = 0;
	float Tiempo = 0.f;

	void FijarEstado(EEstadoManifestacion E);
	int32 TamanoPorApoyo() const;
	int32 PoliciasCerca() const;
	void ActualizarObjetivos();
	void DespawnTodos();
	void AplicarApoyo(float Delta);
	void SubirBusqueda(int32 N);
};
