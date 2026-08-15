// AlsasuaTejadoModular.cpp
#include "AlsasuaTejadoModular.h"
#include "EdificioGenerado.h"
#include "World/AlsasuaMallaFab.h"
#include "Components/HierarchicalInstancedStaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Engine/StaticMesh.h"
#include "Engine/World.h"
#include "EngineUtils.h"
#include "GameFramework/Actor.h"
#include "Math/RandomStream.h"

namespace
{
	// Tipos que resuelve AlsasuaMallaFab contra la familia Roof_ del kit.
	const TCHAR* TipoAlero      = TEXT("tejado_alero");
	const TCHAR* TipoEsquinaExt  = TEXT("tejado_esquina_ext");
	const TCHAR* TipoEsquinaInt  = TEXT("tejado_esquina_int");
	const TCHAR* TipoCumbrera    = TEXT("tejado_cumbrera");
	const TCHAR* TipoCumbreraFin = TEXT("tejado_cumbrera_fin");
	const TCHAR* TipoLimatesa    = TEXT("tejado_limatesa");
	const TCHAR* TipoChimenea    = TEXT("chimenea");
}

UAlsasuaTejadoModular::FPieza* UAlsasuaTejadoModular::Obtener(const FString& Tipo)
{
	if (FPieza* Guardada = Piezas.Find(Tipo))
	{
		return Guardada->ISM ? Guardada : nullptr;
	}

	// Sin forma básica de respaldo: si el kit no está, no se coloca nada.
	UStaticMesh* Malla = AlsasuaMallaFab::Resolver(Tipo, nullptr);

	FPieza P;
	if (!Malla || !Host)
	{
		Piezas.Add(Tipo, P);   // se recuerda el fallo, no se vuelve a buscar
		return nullptr;
	}

	// El largo se mide, no se supone: así el teselado cuadra con el kit que
	// haya, y no hay una constante que se quede vieja al cambiar de pack.
	const FBox Caja = Malla->GetBoundingBox();
	const FVector Tam = Caja.GetSize();
	P.bRunEnY = Tam.Y > Tam.X;
	P.Largo = FMath::Max(P.bRunEnY ? Tam.Y : Tam.X, 10.f);
	P.Ancla = FVector(Caja.GetCenter().X, Caja.GetCenter().Y, Caja.Min.Z);

	P.ISM = NewObject<UHierarchicalInstancedStaticMeshComponent>(Host);
	P.ISM->SetStaticMesh(Malla);
	P.ISM->SetCullDistances(0, FMath::Max(1000, (int32)DistanciaCulling));
	P.ISM->SetCastShadow(true);   // la línea de sombra del alero es medio efecto
	P.ISM->RegisterComponent();
	P.ISM->AttachToComponent(Host->GetRootComponent(), FAttachmentTransformRules::KeepWorldTransform);

	return &Piezas.Add(Tipo, P);
}

void UAlsasuaTejadoModular::ColocarUna(FPieza& P, const FVector& Punto, const FVector& Direccion,
                                       float EscalaLargo)
{
	if (!P.ISM) return;

	FVector Fwd = Direccion.GetSafeNormal();
	if (Fwd.IsNearlyZero()) Fwd = FVector::ForwardVector;

	// Arriba perpendicular al recorrido: en una limatesa el recorrido sube, y
	// con el Z del mundo la pieza saldría cizallada respecto al faldón.
	FVector Arriba = FVector::UpVector - Fwd * Fwd.Z;
	if (Arriba.IsNearlyZero()) Arriba = FVector::ForwardVector;   // recorrido vertical
	Arriba.Normalize();

	const FQuat Rot = (P.bRunEnY ? FRotationMatrix::MakeFromYZ(Fwd, Arriba)
	                             : FRotationMatrix::MakeFromXZ(Fwd, Arriba)).ToQuat();
	const FVector Escala = P.bRunEnY ? FVector(1.f, EscalaLargo, 1.f)
	                                 : FVector(EscalaLargo, 1.f, 1.f);

	FTransform Xf;
	Xf.SetRotation(Rot);
	Xf.SetScale3D(Escala);
	// El ancla de la malla (centro en horizontal, base en vertical) cae en Punto.
	Xf.SetLocation(Punto - Rot.RotateVector(P.Ancla * Escala));

	P.ISM->AddInstance(Xf, /*bWorldSpace=*/true);
	++PiezasColocadas;
}

void UAlsasuaTejadoModular::TenderPiezas(FPieza& P, const FVector& A, const FVector& B)
{
	const FVector AB = B - A;
	const float L = AB.Size();

	// Tramos más cortos que un tercio de pieza se dejan sin tender: una pieza
	// comprimida al 5% es una astilla que se ve, y esos tramos son jogs del
	// footprint de medio metro que la pieza de esquina ya tapa. Con los 1030
	// footprints reales son 224 de 12608 hiladas.
	if (L < P.Largo * 0.35f) return;

	// Se reparte en el número entero de piezas más cercano y se estira el paso:
	// así la hilada llena el tramo exacto y no deja junta abierta al final.
	const int32 Num = FMath::Max(1, FMath::RoundToInt(L / P.Largo));
	const float Paso = L / Num;
	const FVector Dir = AB / L;

	for (int32 i = 0; i < Num; ++i)
	{
		ColocarUna(P, A + Dir * (Paso * (i + 0.5f)), Dir, Paso / P.Largo);
	}
}

void UAlsasuaTejadoModular::RematarEdificio(AEdificioGenerado* Edificio)
{
	if (!Edificio) return;

	const FTejadoConstruido& T = Edificio->Tejado;
	const int32 N = T.Poligono.Num();
	if (N < 3 || T.AlturaCm <= 0.f) return;

	// Perímetro y doble del área firmada en una pasada. El signo del área da el
	// sentido de giro: sin él la normal de alero apunta al interior en la mitad
	// de los edificios, porque el footprint del LIDAR no viene orientado.
	float Perimetro = 0.f;
	float Area2 = 0.f;
	FVector2D Min2 = T.Poligono[0];
	FVector2D Max2 = T.Poligono[0];
	for (int32 i = 0; i < N; ++i)
	{
		const FVector2D& A = T.Poligono[i];
		const FVector2D& B = T.Poligono[(i + 1) % N];
		Perimetro += (B - A).Size();
		Area2 += A.X * B.Y - B.X * A.Y;
		Min2.X = FMath::Min(Min2.X, A.X); Min2.Y = FMath::Min(Min2.Y, A.Y);
		Max2.X = FMath::Max(Max2.X, A.X); Max2.Y = FMath::Max(Max2.Y, A.Y);
	}
	if (Perimetro < PerimetroMinimo) return;
	const float Sentido = (Area2 >= 0.f) ? 1.f : -1.f;   // +1 = antihorario

	const FMetricasTejado Met = CalcularMetricasTejado(T.Poligono, T.AlturaCm, T.Forma, T.Escala);
	const FTransform& Xf = Edificio->GetActorTransform();
	auto Mundo = [&Xf](const FVector2D& P, float Z)
	{
		return Xf.TransformPosition(FVector(P.X, P.Y, Z));
	};

	// Cumbrera, calculada igual que AEdificioGenerado::TejadoDosAguas: eje
	// LIDAR por el centro del bounding box, recortado a la proyección de los
	// vértices. A cuatro aguas degenera en el punto de la cúspide, y entonces
	// las limatesas salen solas desde cada vértice.
	const FVector2D Centro = (Min2 + Max2) * 0.5f;
	FVector2D Eje = T.Eje.GetSafeNormal();
	if (Eje.IsNearlyZero()) Eje = FVector2D(1, 0);

	FVector2D R0 = Centro;
	FVector2D R1 = Centro;
	if (T.Forma == EFormaTejado::Dos_Aguas)
	{
		float tmin = FVector2D::DotProduct(T.Poligono[0] - Centro, Eje);
		float tmax = tmin;
		for (const FVector2D& V : T.Poligono)
		{
			const float t = FVector2D::DotProduct(V - Centro, Eje);
			tmin = FMath::Min(tmin, t);
			tmax = FMath::Max(tmax, t);
		}
		R0 = Centro + Eje * tmin;
		R1 = Centro + Eje * tmax;
	}

	auto EnCumbrera = [&R0, &R1](const FVector2D& Q)
	{
		const FVector2D AB = R1 - R0;
		const float L2 = FVector2D::DotProduct(AB, AB);
		if (L2 <= 1.f) return R0;
		const float t = FMath::Clamp(FVector2D::DotProduct(Q - R0, AB) / L2, 0.f, 1.f);
		return R0 + AB * t;
	};

	FPieza* Alero      = Obtener(TipoAlero);
	FPieza* EsquinaExt = Obtener(TipoEsquinaExt);
	FPieza* EsquinaInt = Obtener(TipoEsquinaInt);
	FPieza* Cumbrera   = Obtener(TipoCumbrera);
	FPieza* CumbreraFin = Obtener(TipoCumbreraFin);
	FPieza* Limatesa   = Obtener(TipoLimatesa);

	// Las esquinas ocupan su propia celda: la hilada de alero se retira media
	// pieza de esquina en cada extremo para no solaparse con ellas.
	const float Retiro = EsquinaExt ? EsquinaExt->Largo * 0.5f : 0.f;

	for (int32 i = 0; i < N; ++i)
	{
		const FVector2D& A = T.Poligono[i];
		const FVector2D& B = T.Poligono[(i + 1) % N];
		const float L = (B - A).Size();
		if (L < 1.f) continue;
		const FVector2D Dir = (B - A) / L;

		// Normal exterior de la arista: a la derecha si el giro es antihorario.
		const FVector2D Fuera = FVector2D(Dir.Y, -Dir.X) * Sentido;

		if (Alero)
		{
			const float Corte = (L > Retiro * 3.f) ? Retiro : 0.f;
			const FVector2D Desp = Fuera * VueloAlero;
			TenderPiezas(*Alero,
				Mundo(A + Dir * Corte + Desp, Met.Alero),
				Mundo(B - Dir * Corte + Desp, Met.Alero));
		}

		// Esquina en el vértice B, entre esta arista y la siguiente.
		const FVector2D& C = T.Poligono[(i + 2) % N];
		const float LSig = (C - B).Size();
		if (LSig < 1.f) continue;
		const FVector2D DirSig = (C - B) / LSig;

		// Producto cruzado con el sentido de giro: convexa o entrante.
		const float Cruz = (Dir.X * DirSig.Y - Dir.Y * DirSig.X) * Sentido;
		if (FMath::Abs(Cruz) < 0.08f) continue;   // vértice casi alineado, no hay esquina

		FPieza* Esquina = (Cruz > 0.f) ? EsquinaExt : EsquinaInt;
		if (!Esquina) continue;

		const FVector2D FueraSig = FVector2D(DirSig.Y, -DirSig.X) * Sentido;
		FVector2D Bisectriz = (Fuera + FueraSig).GetSafeNormal();
		if (Bisectriz.IsNearlyZero()) Bisectriz = Fuera;

		ColocarUna(*Esquina, Mundo(B + Bisectriz * VueloAlero, Met.Alero),
			Xf.TransformVector(FVector(Dir.X, Dir.Y, 0.f)), 1.f);
	}

	// Cumbrera: sólo existe si el eje tiene largo (a cuatro aguas es un punto).
	const float LargoCumbrera = (R1 - R0).Size();
	if (Met.RoofH > 1.f && LargoCumbrera > 1.f)
	{
		if (Cumbrera)
		{
			TenderPiezas(*Cumbrera, Mundo(R0, T.AlturaCm), Mundo(R1, T.AlturaCm));
		}
		if (CumbreraFin)
		{
			const FVector2D DirC = (R1 - R0) / LargoCumbrera;
			const FVector Dir3 = Xf.TransformVector(FVector(DirC.X, DirC.Y, 0.f));
			ColocarUna(*CumbreraFin, Mundo(R0, T.AlturaCm), -Dir3, 1.f);
			ColocarUna(*CumbreraFin, Mundo(R1, T.AlturaCm), Dir3, 1.f);
		}
	}

	// Limatesas: la lima que va de cada vértice a su punto de cumbrera es
	// exactamente el pliegue entre los dos faldones que ahí se encuentran.
	if (Limatesa && Met.RoofH > 1.f)
	{
		for (const FVector2D& V : T.Poligono)
		{
			const FVector2D P = EnCumbrera(V);
			if ((P - V).Size() < 20.f) continue;   // vértice bajo la cumbrera
			TenderPiezas(*Limatesa, Mundo(V, Met.Alero), Mundo(P, T.AlturaCm));
		}
	}

	// Chimenea sobre la cumbrera, sorteada por id para que no baile entre
	// arranques. Las cubiertas planas no llevan.
	if (Met.RoofH > 1.f && FraccionChimenea > 0.f)
	{
		FRandomStream Sorteo(Edificio->Id * 2654435761u + 97);
		if (Sorteo.GetFraction() < FraccionChimenea)
		{
			if (FPieza* Chimenea = Obtener(TipoChimenea))
			{
				// Sobre el faldón, cerca de la cumbrera: se sube por la lima de
				// un vértice cualquiera, así la cota sale del propio faldón y
				// vale igual a dos aguas que a cuatro (donde la cumbrera es un
				// punto y poner la chimenea ahí sería clavarla en la cúspide).
				const FVector2D& V = T.Poligono[Sorteo.RandHelper(N)];
				const float f = Sorteo.FRandRange(0.6f, 0.85f);
				const FVector2D Q = V + (EnCumbrera(V) - V) * f;
				// 15 cm enterrada: el faldón está inclinado y una base plana
				// apoyada justo en la cota deja luz por el lado de abajo.
				ColocarUna(*Chimenea, Mundo(Q, Met.Alero + Met.RoofH * f - 15.f),
					Xf.TransformVector(FVector(Eje.X, Eje.Y, 0.f)), 1.f);
			}
		}
	}

	++EdificiosRematados;
}

int32 UAlsasuaTejadoModular::Cargar()
{
	if (bHecho) return 0;
	bHecho = true;

	UWorld* W = GetWorld();
	if (!W) return 0;

	// Sin pieza de alero no hay kit importado: no se crea ni el actor host.
	if (!AlsasuaMallaFab::Resolver(FString(TipoAlero), nullptr))
	{
		UE_LOG(LogTemp, Log,
			TEXT("[Tejado] kit modular no encontrado (familia Roof_ de Village); tejados sin remate"));
		return 0;
	}

	Host = W->SpawnActor<AActor>();
	if (!Host) return 0;
#if WITH_EDITOR
	Host->SetActorLabel(TEXT("Tejados_Kit_Alsasua"));
#endif
	USceneComponent* Raiz = NewObject<USceneComponent>(Host, TEXT("Root"));
	Raiz->RegisterComponent();
	Host->SetRootComponent(Raiz);

	for (TActorIterator<AEdificioGenerado> It(W); It; ++It)
	{
		RematarEdificio(*It);
	}

	int32 Tipos = 0;
	for (const TPair<FString, FPieza>& Par : Piezas) { if (Par.Value.ISM) ++Tipos; }

	UE_LOG(LogTemp, Log, TEXT("[Tejado] %d piezas de kit en %d edificios (%d/%d tipos resueltos)"),
		PiezasColocadas, EdificiosRematados, Tipos, Piezas.Num());
	return PiezasColocadas;
}
