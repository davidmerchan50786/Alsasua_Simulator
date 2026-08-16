// AjusteMallaComun.h (capa CORE)
// Escala, giro y apoyo para colocar una malla descargada a su tamaño real.
//
// Lo que se baja de Fab, de packs de Unity o de un generador no viene
// normalizado: puede estar en metros, en centímetros o en las unidades del
// programa con el que se exportó, con el eje largo en X o en Y y con el origen
// en el centro, en la base o donde le tocara. Colocarlo tal cual da coches del
// tamaño de una casa o señales enterradas.
//
// Aquí se mide el bounding box y se saca de él lo que hace falta para que la
// pieza mida lo que mide de verdad. Vive en Core porque lo necesitan el
// ferrocarril, el tráfico y cualquier otro sistema que coloque assets de origen
// desconocido: el problema es de la malla, no del sistema que la usa.
#pragma once

#include "CoreMinimal.h"
#include "Engine/StaticMesh.h"

namespace AjusteMalla
{
	/** Qué medida manda cuando se escala uniforme. */
	enum class EEncaje : uint8
	{
		Largo,   // vehículos, vagones: lo que define el tamaño es la longitud
		Alto,    // señales, farolas, postes: lo que define el tamaño es la altura
	};

	struct FColocacion
	{
		FVector Escala = FVector::OneVector;
		/** Grados a sumar al yaw para que el eje largo apunte donde debe. */
		float   YawExtra = 0.f;
		/** Cuánto subir el actor para que su base quede en la cota de apoyo. */
		float   SubirCm = 0.f;
		bool    bValido = false;
	};

	/**
	 * @param Malla      La que se va a colocar.
	 * @param MedidasM   Tamaño real (largo, ancho, alto) en metros.
	 * @param bUniforme  true para una malla modelada de verdad: se escala igual en
	 *                   los tres ejes, porque estirar un modelo para cuadrarle una
	 *                   medida lo estropea. false para una forma básica del motor,
	 *                   donde encajar las tres medidas da una silueta que al menos
	 *                   se lee como lo que pretende ser.
	 * @param Encaje     Qué medida manda con bUniforme.
	 */
	inline FColocacion Calcular(const UStaticMesh* Malla, const FVector& MedidasM,
	                            bool bUniforme, EEncaje Encaje = EEncaje::Largo)
	{
		FColocacion C;
		if (!Malla) return C;

		const FBoxSphereBounds B = Malla->GetBounds();
		const double Ex = B.BoxExtent.X, Ey = B.BoxExtent.Y, Ez = B.BoxExtent.Z;
		// Media unidad de semi-extensión: por debajo de eso la malla no tiene
		// volumen medible y cualquier factor que salga es basura.
		if (FMath::Max(Ex, Ey) < 0.5 || Ez < 0.5) return C;

		const bool bLargoEnY = (Ey > Ex);
		C.YawExtra = bLargoEnY ? 90.f : 0.f;

		const double EscLargo = MedidasM.X * 100.0 / (2.0 * FMath::Max(Ex, Ey));
		const double EscAncho = MedidasM.Y * 100.0 / (2.0 * FMath::Min(Ex, Ey));
		const double EscAlto  = MedidasM.Z * 100.0 / (2.0 * Ez);

		if (bUniforme)
		{
			C.Escala = FVector((float)(Encaje == EEncaje::Alto ? EscAlto : EscLargo));
		}
		else
		{
			C.Escala = bLargoEnY
				? FVector((float)EscAncho, (float)EscLargo, (float)EscAlto)
				: FVector((float)EscLargo, (float)EscAncho, (float)EscAlto);
		}

		// El origen rara vez está en la base de la malla; esto es lo que hay que
		// subir el actor para que la base acabe justo en la cota de apoyo.
		C.SubirCm = (float)(-(B.Origin.Z - Ez) * C.Escala.Z);
		C.bValido = true;
		return C;
	}
}
