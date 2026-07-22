// DrogasSubsystem.cpp
#include "DrogasSubsystem.h"
#include "ApoyoPopularSubsystem.h"

void UDrogasSubsystem::Tomar(ESustancia S)
{
	Activa = S;
	UApoyoPopularSubsystem* Ap = GetGameInstance() ? GetGameInstance()->GetSubsystem<UApoyoPopularSubsystem>() : nullptr;
	switch (S)
	{
	case ESustancia::Porro: TiempoRestante = 35.f; MultDisp = 1.3f; ReduDano = 1.f;  MultVel = 0.95f; if (Ap) Ap->RestarParanoia(15.f); break;
	case ESustancia::Speed: TiempoRestante = 30.f; MultDisp = 1.15f;ReduDano = 1.f;  MultVel = 1.25f; if (Ap) Ap->SumarParanoia(10.f); break;
	case ESustancia::Chute: TiempoRestante = 45.f; MultDisp = 1.4f; ReduDano = 0.5f; MultVel = 0.7f;  if (Ap) Ap->RestarParanoia(25.f); break;
	case ESustancia::Tripi: TiempoRestante = 60.f; MultDisp = 1.9f; ReduDano = 1.f;  MultVel = 0.85f; break;
	default: break;
	}
}

void UDrogasSubsystem::Bajada()
{
	if (Activa == ESustancia::Speed)
		if (UApoyoPopularSubsystem* Ap = GetGameInstance() ? GetGameInstance()->GetSubsystem<UApoyoPopularSubsystem>() : nullptr)
			Ap->SumarParanoia(18.f);   // el bajón del speed deja paranoia
	Activa = ESustancia::Ninguna;
	MultDisp = 1.f; ReduDano = 1.f; MultVel = 1.f;
}

void UDrogasSubsystem::Tick(float DeltaTime)
{
	if (Borrachera > 0.f) Borrachera = FMath::Max(0.f, Borrachera - DeltaTime * 1.5f);
	if (Activa == ESustancia::Ninguna) return;
	TiempoRestante -= DeltaTime;
	if (TiempoRestante <= 0.f) Bajada();
}
