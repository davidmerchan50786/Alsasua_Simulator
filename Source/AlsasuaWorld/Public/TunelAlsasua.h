// TunelAlsasua.h (capa WORLD)
// Bocas de túnel sobre el trazado de tunnels_unity.json.
//
// El dataset trae los cinco túneles reales del corredor: los dos ferroviarios
// (Txantxiku y el de la línea de Castejón), el de la N-1 en el puerto, el de la
// autovía A-10 por la Sakana y el histórico del Plazaola, hoy vía verde. Con su
// trazado, su ancho y su longitud declarada.
//
// UCargadorVias los encolaba y PasoPresupuesto los descartaba con un comentario
// que decía "placeholder hasta ATunelAlsasua": los datos se leían, se contaban
// como construidos y no se dibujaba nada.
//
// QUÉ HACE Y QUÉ NO. Construye las dos bocas de cada túnel: el marco de hormigón
// con su arco, apoyado en el terreno y orientado según el trazado. Eso es lo que
// se ve desde fuera y es geometría real.
//
// Lo que NO hace es agujerear el terreno, así que el túnel no se puede
// atravesar: se ven sus bocas en la ladera y por dentro no hay nada. Cavar la
// galería sin poder entrar en ella sería geometría enterrada que nadie ve
// pagando draw calls.
//
// SI ALGUIEN SE PLANTEA CAVARLA, éstos son los números, medidos sobre el dato y
// sobre los parámetros del terreno, para no volver a decidirlo a ojo:
//
//   · Galería total de los cinco túneles: 4187 m de trazado.
//   · Malla de la galería: a 4 m por anillo y 8 vértices por anillo son ~8400
//     vértices. Nada — las fachadas del pueblo son 60 000 en una sección.
//   · Hueco en el terreno: las galerías pasan por debajo de unos 9145 quads de
//     los 16 257 024 del terreno, o sea el 0,056%, repartidos en ~26 chunks de
//     los 1024. El terreno ya se construye chunk a chunk emitiendo triángulos,
//     así que saltarse los de dentro del corredor es un filtro local, no una
//     reconstrucción.
//
// O sea que el coste de malla NO es la razón para no hacerlo; es mucho menor de
// lo que sugiere "una malla de 4033²". Lo que de verdad cuesta es el ORDEN: el
// terreno se construye en la fase 1 y los túneles en la 51b, así que para
// recortar el hueco el terreno tendría que conocer los corredores antes de
// existir. Eso es invertir una dependencia en el sistema más básico del
// arranque, y hacerlo mal deja un agujero por el que se cae el jugador.
//
// Y sigue siendo decisión de producto: un túnel atravesable necesita galería,
// colisión, iluminación y una razón para entrar. Los números de arriba dicen
// que se puede; no dicen que merezca la pena.
#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "TunelAlsasua.generated.h"

class UProceduralMeshComponent;

UCLASS()
class ALSASUAWORLD_API ATunelAlsasua : public AActor
{
	GENERATED_BODY()

public:
	ATunelAlsasua();

	/** Lee el dataset y levanta las bocas. Devuelve cuántas ha puesto. */
	int32 Construir();

	/** Altura libre de la boca sobre el firme (cm). */
	UPROPERTY(EditAnywhere, Category="Tunel") float AlturaLibreCm = 620.f;

	/** Grosor del marco de hormigón alrededor del vano (cm). */
	UPROPERTY(EditAnywhere, Category="Tunel") float MarcoCm = 120.f;

	/** Segmentos del arco. Ocho ya se lee redondo a la distancia a la que se ve. */
	UPROPERTY(EditAnywhere, Category="Tunel") int32 SegmentosArco = 8;

	UPROPERTY(VisibleAnywhere) UProceduralMeshComponent* Malla;

private:
	/**
	 * Añade una boca al buffer común.
	 *
	 * @param CentroCm  Punto del trazado donde va la boca (mundo, cm).
	 * @param DirXY     Dirección del túnel ahí, normalizada.
	 * @param AnchoCm   Ancho del vano.
	 */
	void AnadirBoca(const FVector& CentroCm, const FVector2D& DirXY, float AnchoCm,
	                TArray<FVector>& V, TArray<int32>& T, TArray<FVector>& N,
	                TArray<FVector2D>& UV, TArray<FColor>& C);
};
