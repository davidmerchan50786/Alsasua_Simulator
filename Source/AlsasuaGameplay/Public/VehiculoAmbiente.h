// VehiculoAmbiente.h (capa GAMEPLAY)
// Vehículo ambiental kinemático: recorre un eje de calle a velocidad constante,
// drapeado sobre el terreno. Sin física (tráfico de relleno). Puerto de la parte
// de coches de SistemaTrafico/VehiculoNPC.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "AlsasuaTypes.h"
#include "VehiculoAmbiente.generated.h"

class UStaticMeshComponent;
class UNiagaraComponent;

UCLASS()
class ALSASUAGAMEPLAY_API AVehiculoAmbiente : public AActor, public IDamageable
{
	GENERATED_BODY()

public:
	AVehiculoAmbiente();

	// ── Daño (IDamageable) ──
	UPROPERTY(EditAnywhere, Category="Coche") int32 Vida = 120;
	UPROPERTY(EditAnywhere, Category="Coche") int32 VidaMaxima = 120;
	virtual int32 GetVida() const override    { return Vida; }
	virtual int32 GetVidaMax() const override { return VidaMaxima; }
	virtual bool  EstaMuerto() const override { return Vida <= 0; }
	virtual void  Curar(int32 C) override      { Vida = FMath::Min(VidaMaxima, Vida + C); }
	virtual void  RecibirDano(int32 C, FVector Origen, ETipoDano Tipo) override;

	// Ruta en mundo XY (cm), ancho de calzada (cm), índice de inicio,
	// sentido (+1 hacia índices crecientes, -1 decrecientes) y velocidad (cm/s).
	void Iniciar(const TArray<FVector2D>& Eje, float AnchoCalzadaCm, int32 IndiceInicio, int32 Sentido, float VelocidadCmS);

	bool Terminado() const { return bTerminado; }

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Cuerpo;

protected:
	virtual void Tick(float DeltaTime) override;

private:
	TArray<FVector2D> Ruta;   // ya orientada al sentido de marcha
	int32 Seg = 0;
	float DistEnSeg = 0.f;
	float Velocidad = 1000.f;
	float OffsetCarril = 150.f;   // desplazamiento al carril derecho (cm)
	bool bTerminado = false;
	bool bHumo = false, bExplotado = false;
	UPROPERTY() UNiagaraComponent* Humo = nullptr;

	float AlturaSuelo(const FVector2D& XY) const;
	void Explotar();
};
