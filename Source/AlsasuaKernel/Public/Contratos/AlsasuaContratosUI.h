#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "Delegates/DelegateCombinations.h"
#include "AlsasuaContratosUI.generated.h"

/** Evento global de mundo (antes FOnDirectorAction, acoplado a GF_Systems). */
DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FAlsasuaEventoMundo, FText, Descripcion);

/**
 * Contratos tronco<->pilares para UI y datos de fachada. El tronco consume
 * estos contratos sin conocer tipos concretos de ningun plugin; cada pilar
 * implementa los suyos.
 */

/** Emisor de eventos de mundo (GF_Systems). */
UINTERFACE(MinimalAPI)
class UAlsasuaFuenteEventosMundo : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaFuenteEventosMundo
{
	GENERATED_BODY()

public:
	virtual FAlsasuaEventoMundo& EventoMundo() = 0;
};

/** Red social simulada (GF_Social). */
UINTERFACE(MinimalAPI)
class UAlsasuaRedSocial : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaRedSocial
{
	GENERATED_BODY()

public:
	virtual float SeguidoresGlobales() const { return 0.f; }
};

/** Ajustes graficos del pilar de mundo (GF_World). */
UINTERFACE(MinimalAPI)
class UAlsasuaAjustesGraficos : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaAjustesGraficos
{
	GENERATED_BODY()

public:
	/** Perfil 0=Low .. 3=Ultra. */
	virtual void AplicarPerfilGrafico(int32 Perfil) {}
};

/** Datos de fachada por edificio (GF_Edificios). */
UINTERFACE(MinimalAPI)
class UAlsasuaDatosFachadas : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaDatosFachadas
{
	GENERATED_BODY()

public:
	/** Medidas en metros para vestir un edificio; false si no hay dato. */
	virtual bool MedidasDe(int32 IdEdificio, float& AlturaPorNivelM,
		float& AnchoVentanaM, float& AltoVentanaM) const { return false; }
};

/** Marcador: "yo soy el dueno del cielo" (GF_Clima). */
UINTERFACE(MinimalAPI)
class UAlsasuaDuenoCielo : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaDuenoCielo
{
	GENERATED_BODY()
};

/**
 * Respaldo de vegetacion del tronco: el pintor HISM del pilar lo usa cuando
 * el pack Naturaleza no tiene mallas, para que las zonas verdes no queden
 * peladas. Nunca pintor Y respaldo a la vez.
 */
UINTERFACE(MinimalAPI)
class UAlsasuaRespaldoVegetacion : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaRespaldoVegetacion
{
	GENERATED_BODY()

public:
	virtual int32 SembrarVegetacion() { return 0; }
};

/**
 * Estado del clima publicado por el pilar de clima y consumido por el de
 * audio (antes el DirectorArranque hacia de puente cada tick).
 */
UINTERFACE(MinimalAPI)
class UAlsasuaEstadoClima : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaEstadoClima
{
	GENERATED_BODY()

public:
	virtual bool EstaLloviendo() const { return false; }
	virtual bool HayTormenta() const { return false; }
	virtual float HoraDeJuego() const { return 12.f; }
	virtual float VelocidadViento() const { return 0.f; }
	virtual float IntensidadLluvia() const { return 0.f; }
};
