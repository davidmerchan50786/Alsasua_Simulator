// AlsasuaNPC.h (capa ENTITIES)
// NPC base con vida (IDamageable). Puerto mínimo de NPCBase/PoliciaForalIA.
// La IA (percepción, GOAP, behavior tree) se conecta aparte con AIModule.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "AlsasuaTypes.h"
#include "AlsasuaNPC.generated.h"

UCLASS()
class ALSASUAENTITIES_API AAlsasuaNPC : public ACharacter, public IDamageable
{
	GENERATED_BODY()

public:
	AAlsasuaNPC();

	UPROPERTY(BlueprintReadOnly, Category="Vida") int32 Vida = 100;
	UPROPERTY(EditAnywhere,      Category="Vida") int32 VidaMaxima = 100;
	UPROPERTY(EditAnywhere,      Category="NPC")  bool  bEsPolicia = false;   // civil vs autoridad
	UPROPERTY(EditAnywhere,      Category="NPC")  bool  bEsManifestante = false;  // protest participant
	UPROPERTY(BlueprintReadOnly, Category="Vida") bool  bMuerto = false;
	UPROPERTY(EditAnywhere,      Category="NPC")  float DuracionCadaver = 30.f;   // s antes de hundirse y limpiar
	UPROPERTY(EditAnywhere,      Category="NPC")  float ImpulsoMuerte = 12000.f;  // empuje del ragdoll

	virtual int32 GetVida() const override    { return Vida; }
	virtual int32 GetVidaMax() const override { return VidaMaxima; }
	virtual bool  EstaMuerto() const override { return bMuerto; }
	virtual void  Curar(int32 Cantidad) override { if (!bMuerto) Vida = FMath::Min(VidaMaxima, Vida + Cantidad); }
	virtual void  RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo) override;

protected:
	FVector UltimoOrigenDano = FVector::ZeroVector;
	void Morir();
};
