#pragma once

#include "CoreMinimal.h"
#include "UObject/Interface.h"
#include "AlsasuaPilarArranque.generated.h"

/**
 * Contrato de arranque de un pilar GF_*.
 *
 * El DirectorArranque ya no conoce tipos concretos de ningun pilar: recorre
 * los subsistemas del mundo y del GameInstance, y a todo el que implemente
 * esta interfaz le pide que ejecute SU fase de construccion. Un pilar
 * desactivado simplemente no esta: su fase no corre y el resto sigue.
 *
 * El orden historico de las fases se preserva con OrdenArranque() (usa el
 * numero de la fase original del Director como prioridad; menos = antes).
 */
UINTERFACE(MinimalAPI)
class UAlsasuaPilarArranque : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaPilarArranque
{
	GENERATED_BODY()

public:
	/** Ejecuta la fase de arranque del pilar. Devuelve el conteo para el log
	 *  del director, o -1 si la fase no reporta cifra. */
	virtual int32 EjecutarArranque() { return -1; }

	/** Etiqueta corta para el log: "farolas con control automatico". */
	virtual FString EtiquetaArranque() const { return TEXT("pilar sin etiqueta"); }

	/** Prioridad dentro de la cadena de arranque (menos = antes). Usa el
	 *  numero de la fase original del DirectorArranque para conservar el
	 *  orden historico. */
	virtual int32 OrdenArranque() const { return 500; }
};

/**
 * Contrato de tiqueo por pilar: lo que antes el DirectorArranque avanzaba a
 * mano (peatones, trafico, puente clima->audio) ahora cada pilar lo avanza.
 */
UINTERFACE(MinimalAPI)
class UAlsasuaPilarTiquear : public UInterface
{
	GENERATED_BODY()
};

class ALSASUAKERNEL_API IAlsasuaPilarTiquear
{
	GENERATED_BODY()

public:
	virtual void TiquearPilar(float DeltaTime) {}
};

/** Flags de configuracion de arranque compartidos sin acoplar modulos:
 *  el Director los escribe, los pilares los leen. */
namespace AlsasuaArranqueFlags
{
	extern ALSASUAKERNEL_API bool bSemaforos;
}
