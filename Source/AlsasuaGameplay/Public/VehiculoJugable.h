// VehiculoJugable.h (capa GAMEPLAY)
// Coche conducible arcade (sin Chaos Vehicles ni assets de rueda): el jugador
// lo posee al entrar. Acelera/frena, gira según velocidad y se drapea sobre el
// terreno. Puerto jugable de VehiculoBase/SistemaTrafico (conducción).
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Pawn.h"
#include "AlsasuaTypes.h"
#include "VehiculoJugable.generated.h"

class UStaticMeshComponent;
class USpringArmComponent;
class UCameraComponent;
class UNiagaraComponent;

UCLASS()
class ALSASUAGAMEPLAY_API AVehiculoJugable : public APawn, public IDamageable
{
	GENERATED_BODY()

public:
	AVehiculoJugable();

	// ── Daño (IDamageable) ──
	UPROPERTY(EditAnywhere, Category="Coche") int32 Vida = 200;
	UPROPERTY(EditAnywhere, Category="Coche") int32 VidaMaxima = 200;
	virtual int32 GetVida() const override    { return Vida; }
	virtual int32 GetVidaMax() const override { return VidaMaxima; }
	virtual bool  EstaMuerto() const override { return Vida <= 0; }
	virtual void  Curar(int32 C) override      { Vida = FMath::Min(VidaMaxima, Vida + C); }
	virtual void  RecibirDano(int32 C, FVector Origen, ETipoDano Tipo) override;

	UPROPERTY(EditAnywhere, Category="Coche") float VelMax = 1800.f;     // cm/s (~65 km/h)
	UPROPERTY(EditAnywhere, Category="Coche") float Aceleracion = 900.f;
	UPROPERTY(EditAnywhere, Category="Coche") float GiroGrados = 90.f;   // deg/s a tope
	UPROPERTY(EditAnywhere, Category="Coche") float Rozamiento = 600.f;

	UPROPERTY(VisibleAnywhere) UStaticMeshComponent* Cuerpo;
	UPROPERTY(VisibleAnywhere) USpringArmComponent* Brazo;
	UPROPERTY(VisibleAnywhere) UCameraComponent* Cam;

	virtual void Tick(float DeltaTime) override;
	virtual void SetupPlayerInputComponent(UInputComponent* PlayerInputComponent) override;

	float Velocidad() const { return Vel; }

private:
	float Vel = 0.f;
	float Acelerador = 0.f;   // -1..1
	float Volante = 0.f;      // -1..1
	bool  bFreno = false;

	void Acelerar(float V) { Acelerador = V; }
	void Girar(float V)    { Volante = V; }
	void FrenoOn()  { bFreno = true; }
	void FrenoOff() { bFreno = false; }

	float AlturaSuelo(const FVector& XY) const;
	void Atropellar();   // daña a NPCs delante si vamos rápido
	UPROPERTY() TSet<TWeakObjectPtr<AActor>> YaAtropellados;

	bool bHumo = false, bExplotado = false;
	UPROPERTY() UNiagaraComponent* Humo = nullptr;
	void Explotar();
};
