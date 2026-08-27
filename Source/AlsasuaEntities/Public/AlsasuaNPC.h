// AlsasuaNPC.h (capa ENTITIES)
// NPC base con vida (IDamageable), paranoia per-actor, morale, y efectos visuales.
// La IA (percepción, GOAP, behavior tree) se conecta aparte con AIModule.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlsasuaTypes.h"
#include "AlsasuaNPC.generated.h"

class UAlsasuaParanoiaComponent;
class UAlsasuaWhisperManager;

UCLASS()
class ALSASUAENTITIES_API AAlsasuaNPC : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	AAlsasuaNPC();

	// ── Vida ──
	UPROPERTY(BlueprintReadOnly, Category="Vida") int32 Vida = 100;
	UPROPERTY(EditAnywhere,      Category="Vida") int32 VidaMaxima = 100;
	UPROPERTY(EditAnywhere,      Category="NPC")  bool  bEsPolicia = false;   // civil vs autoridad
	UPROPERTY(EditAnywhere,      Category="NPC")  bool  bEsManifestante = false;  // protest participant
	UPROPERTY(BlueprintReadOnly, Category="Vida") bool  bMuerto = false;
	UPROPERTY(EditAnywhere,      Category="NPC")  float DuracionCadaver = 30.f;
	UPROPERTY(EditAnywhere,      Category="NPC")  float ImpulsoMuerte = 12000.f;

	// ── Morale (migrado de ANPCCharacter) ──
	UPROPERTY(BlueprintReadWrite, EditAnywhere, Category="NPC|Morale", meta=(ClampMin="0",ClampMax="100"))
	float Morale = 50.f;

	UFUNCTION(BlueprintCallable, Category="NPC|Morale")
	void SetMorale(float NewMorale) { Morale = FMath::Clamp(NewMorale, 0.f, 100.f); }

	// ── Componentes wiring ──
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Paranoia")
	TObjectPtr<UAlsasuaParanoiaComponent> ParanoiaComp;

	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category="NPC|Audio")
	TObjectPtr<UAlsasuaWhisperManager> WhisperComp;

	// ── IDamageable ──
	virtual int32 GetVida() const override    { return Vida; }
	virtual int32 GetVidaMax() const override { return VidaMaxima; }
	virtual bool  EstaMuerto() const override { return bMuerto; }
	virtual void  Curar(int32 Cantidad) override { if (!bMuerto) Vida = FMath::Min(VidaMaxima, Vida + Cantidad); }
	virtual void  RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo) override;

protected:
	FVector UltimoOrigenDano = FVector::ZeroVector;
	void Morir();

	virtual void BeginPlay() override;

private:
	float LastWhisperThreshold = 0.f;

	UFUNCTION()
	void OnParanoiaLevelChanged(float NewLevel);
};
