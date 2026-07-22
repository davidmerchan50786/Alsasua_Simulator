// GobernadorRender.cpp
#include "GobernadorRender.h"

void UGobernadorRender::Tick(float DeltaTime)
{
	const float Ms = FMath::Max(DeltaTime * 1000.f, 0.1f);
	MsSuave = FMath::Lerp(MsSuave, Ms, 0.1f);   // EMA del frame-time

	// Carga: <1 cuando vamos por encima del presupuesto.
	Carga = FMath::Clamp(MsObjetivo / MsSuave, 0.f, 2.f);

	// Encoge bajo presión, expande con holgura (paso suave por frame).
	const float Objetivo = (Carga < 1.f)
		? FMath::Lerp(RadioMin, Radio, FMath::Clamp(Carga, 0.f, 1.f))   // hacia RadioMin
		: FMath::Min(Radio * 1.02f, RadioMax);                          // crece 2%/frame
	Radio = FMath::Clamp(FMath::Lerp(Radio, Objetivo, 0.2f), RadioMin, RadioMax);
}
