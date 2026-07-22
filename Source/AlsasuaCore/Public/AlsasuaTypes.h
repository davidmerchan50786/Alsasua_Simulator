// AlsasuaTypes.h — tipos y contratos compartidos (capa CORE).
// Puerto de los enums/interfaces de Unity (TipoArma, TipoDano, IDamageable,
// Sustancia, TipoNegocio…). Mismos valores/orden que el proyecto Unity.
#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AlsasuaTypes.generated.h"

UENUM(BlueprintType)
enum class ETipoArma : uint8
{
	Punos, Spray, Tirachinas, Molotov, Ikurrina, BombaLapa, CocheBomba, Pistola, Escopeta, Fusil
};

UENUM(BlueprintType)
enum class ETipoDano : uint8
{
	Bala, Explosion, Fuego, Impacto, Caida
};

UENUM(BlueprintType)
enum class ETipoNegocio : uint8
{
	Bar, Comercio, Empresa, Industria
};

UENUM(BlueprintType)
enum class ESustancia : uint8
{
	Ninguna, Porro, Speed, Chute, Tripi
};

// ── Contrato de daño (equivalente a IDamageable de Unity) ───────────────────
UINTERFACE(MinimalAPI, BlueprintType)
class UDamageable : public UInterface
{
	GENERATED_BODY()
};

class ALSASUACORE_API IDamageable
{
	GENERATED_BODY()
public:
	virtual int32 GetVida() const = 0;
	virtual int32 GetVidaMax() const = 0;
	virtual bool  EstaMuerto() const = 0;
	virtual void  RecibirDano(int32 Cantidad, FVector Origen, ETipoDano Tipo) = 0;
	virtual void  Curar(int32 Cantidad) = 0;
};
