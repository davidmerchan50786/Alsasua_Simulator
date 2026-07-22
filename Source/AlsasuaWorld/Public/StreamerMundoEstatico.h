// StreamerMundoEstatico.h (capa WORLD)
// Clasifica el mundo estático (edificios + props) en 3 bandas según el radio
// dinámico del GobernadorRender: Activo / Impostor-lite / Oculto, con histéresis.
// Puerto de Runtime/StreamerMundoEstatico.cs. No toca árboles ni multitud.
#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "StreamerMundoEstatico.generated.h"

UENUM()
enum class EBandaMundo : uint8 { Activo, Impostor, Oculto };

USTRUCT()
struct FRegistroMundo
{
	GENERATED_BODY()
	UPROPERTY() TWeakObjectPtr<AActor> Actor;
	EBandaMundo Banda = EBandaMundo::Activo;
};

UCLASS()
class ALSASUAWORLD_API UStreamerMundoEstatico : public UWorldSubsystem, public FTickableGameObject
{
	GENERATED_BODY()

public:
	// Tags que recoge como mundo estático (coincide con el spawn del mundo).
	UPROPERTY(EditAnywhere, Category="Streaming") TArray<FName> Tags = { "Edificio", "Prop", "MobiliarioUrbano" };
	UPROPERTY(EditAnywhere, Category="Streaming") float Histeresis = 1500.f;   // margen anti-parpadeo (cm)
	UPROPERTY(EditAnywhere, Category="Streaming") float PeriodoReclasif = 0.2f;

	virtual void OnWorldBeginPlay(UWorld& InWorld) override;

	// Registro manual (lo usan los constructores de mundo al instanciar).
	void Registrar(AActor* A);

	// FTickableGameObject
	virtual void Tick(float DeltaTime) override;
	virtual TStatId GetStatId() const override { RETURN_QUICK_DECLARE_CYCLE_STAT(UStreamerMundoEstatico, STATGROUP_Tickables); }
	virtual bool IsTickable() const override { return !IsTemplate(); }
	virtual bool DoesSupportWorldType(const EWorldType::Type Tipo) const override { return Tipo == EWorldType::Game || Tipo == EWorldType::PIE; }

private:
	UPROPERTY() TArray<FRegistroMundo> Registro;
	float Acum = 0.f;
	void AplicarBanda(FRegistroMundo& R, EBandaMundo Nueva);
	void Reclasificar();
};
