// EconomiaCriminalSubsystem.cpp
#include "EconomiaCriminalSubsystem.h"
#include "NegocioActor.h"
#include "EconomiaSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "ProgresionSubsystem.h"
#include "WantedSubsystem.h"
#include "DiaNocheSubsystem.h"

DEFINE_LOG_CATEGORY_STATIC(LogAlsasuaCrimen, Log, All);

FOnCriminalActivity UEconomiaCriminalSubsystem::OnCriminalActivity;

namespace
{
	template<typename T> T* Sub(const UGameInstanceSubsystem* S)
	{
		return S && S->GetGameInstance() ? S->GetGameInstance()->GetSubsystem<T>() : nullptr;
	}
}

void UEconomiaCriminalSubsystem::Registrar(ANegocioActor* N)
{
	if (N && !Negocios.ContainsByPredicate([N](const TWeakObjectPtr<ANegocioActor>& W){ return W.Get() == N; }))
		Negocios.Add(N);
}

void UEconomiaCriminalSubsystem::Quitar(ANegocioActor* N)
{
	Negocios.RemoveAll([N](const TWeakObjectPtr<ANegocioActor>& W){ return W.Get() == N || !W.IsValid(); });
}

void UEconomiaCriminalSubsystem::Extorsionar(ANegocioActor* N)
{
	if (!N || N->Estado != EEstadoNegocio::Libre) return;
	N->PonerBajoControl();

	UProgresionSubsystem* Prog = Sub<UProgresionSubsystem>(this);
	const float Mult = Prog ? Prog->MultiplicadorIngresos() : 1.f;
	const int32 Inicial = FMath::RoundToInt(N->IngresoMin() * 2 * Mult);

	if (UEconomiaSubsystem* Eco = Sub<UEconomiaSubsystem>(this)) Eco->GanarDinero(Inicial);
	if (UApoyoPopularSubsystem* Ap = Sub<UApoyoPopularSubsystem>(this))
	{
		Ap->SumarParanoia(10.f);
		Ap->RestarApoyo(2.f, TEXT("extorsion"));
	}
	if (UWantedSubsystem* W = Sub<UWantedSubsystem>(this))
		W->AumentarBusqueda(FMath::Max(0, 1 - (Prog ? Prog->ReduccionCalor() : 0)));
	UE_LOG(LogAlsasuaCrimen, Log, TEXT("Extorsion: %s (+%d, paga %d/min)"), *N->Nombre, Inicial, N->IngresoMin());
	OnCriminalActivity.Broadcast(NAME_None, Inicial);
}

void UEconomiaCriminalSubsystem::Trapichear()
{
	if (bCooldownActive) {
		UE_LOG(LogAlsasuaCrimen, Log, TEXT("Trapicheo en cooldown tras redada."));
		return;
	}

	UEconomiaSubsystem* Eco = Sub<UEconomiaSubsystem>(this);
	if (!Eco) return;

	const bool bRedada = FMath::FRand() < 0.2f;
	if (UApoyoPopularSubsystem* Ap = Sub<UApoyoPopularSubsystem>(this))
	{
		Ap->SumarParanoia(15.f);
		Ap->RestarApoyo(3.f, TEXT("trafico"));
	}
	if (UWantedSubsystem* W = Sub<UWantedSubsystem>(this)) W->AumentarBusqueda(bRedada ? 3 : 2);
	if (bRedada) {
		UE_LOG(LogAlsasuaCrimen, Log, TEXT("Redada: pierdes el alijo y quedas sin trapicheo por un periodo."));
		// Apply cooldown: prevent further Trapichear calls for some time.
		bCooldownActive = true;
		CooldownTimer = REDADA_COOLDOWN;
		return;
	}

	UDiaNocheSubsystem* Dn = Sub<UDiaNocheSubsystem>(this);
	const int32 Ganancia = FMath::RoundToInt(FMath::RandRange(150, 400) * (Dn ? Dn->FactorTrapicheo() : 1.f));
	Eco->GanarDinero(Ganancia);
	OnCriminalActivity.Broadcast(FName("Trapicheo"), Ganancia);
}

void UEconomiaCriminalSubsystem::Tick(float DeltaTime)
{
	if (bCooldownActive) {
		CooldownTimer -= DeltaTime;
		if (CooldownTimer <= 0.f) {
			bCooldownActive = false;
			UE_LOG(LogAlsasuaCrimen, Log, TEXT("Cooldown de redada finalizado. Trapicheo disponible."));
		}
		return;
	}

	Acumulado += DeltaTime;
	if (Acumulado < PERIODO) return;
	Acumulado = 0.f;

	UProgresionSubsystem* Prog = Sub<UProgresionSubsystem>(this);
	const float Mult = Prog ? Prog->MultiplicadorIngresos() : 1.f;

	UDiaNocheSubsystem* Dn = Sub<UDiaNocheSubsystem>(this);
	int32 Total = 0;
	for (const TWeakObjectPtr<ANegocioActor>& W : Negocios)
		if (ANegocioActor* N = W.Get())
			if (N->Estado == EEstadoNegocio::Extorsionado)
				Total += FMath::RoundToInt(N->IngresoMin() * (Dn ? Dn->FactorIngresoNegocio(N->Tipo) : 1.f));

	Total = FMath::RoundToInt(Total * Mult);
	if (Total > 0)
		if (UEconomiaSubsystem* Eco = Sub<UEconomiaSubsystem>(this))
		{
			Eco->GanarDinero(Total);
			UE_LOG(LogAlsasuaCrimen, Log, TEXT("Impuesto revolucionario: +%d"), Total);
		}
}
