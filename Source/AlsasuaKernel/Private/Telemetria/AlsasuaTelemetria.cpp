// AlsasuaTelemetria.cpp
#include "Telemetria/AlsasuaTelemetria.h"
#include "Engine/GameInstance.h"
#include "Engine/World.h"
#include "HAL/IConsoleManager.h"
#include "Misc/CommandLine.h"
#include "Modules/ModuleManager.h"
#include "Plugins/AlsasuaCargadorPlugins.h"
#include "Sockets.h"
#include "SocketSubsystem.h"

namespace
{
	int32 FramesDesdeLatido = 0;
	FDelegateHandle GanchoFrame;
}

void UAlsasuaTelemetria::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);

	GanchoFrame = FCoreDelegates::OnBeginFrame.AddLambda(
		[]() { ++FramesDesdeLatido; });

	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().SetTimer(MangoLatido,
			FTimerDelegate::CreateUObject(this, &UAlsasuaTelemetria::Latido),
			1.0f, /*bLoop*/true);
	}
}

void UAlsasuaTelemetria::Deinitialize()
{
	FCoreDelegates::OnBeginFrame.Remove(GanchoFrame);
	if (UWorld* W = GetWorld())
	{
		W->GetTimerManager().ClearTimer(MangoLatido);
	}
	if (Socket)
	{
		Socket->Close();
		ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->DestroySocket(Socket);
		Socket = nullptr;
	}
	Super::Deinitialize();
}

bool UAlsasuaTelemetria::ShouldCreateSubsystem(UObject* Outer) const
{
	if (!Super::ShouldCreateSubsystem(Outer))
	{
		return false;
	}
	// Opt-in explicito: sin parametro y sin ini, no existe (cero coste).
	FString ValorIni;
	bool bActiva = TelemetriaActivada;
	if (FParse::Value(FCommandLine::Get(), TEXT("-AlsasuaTelemetria"), ValorIni))
	{
		bActiva = !ValorIni.Equals(TEXT("0"), ESearchCase::IgnoreCase) &&
				  !ValorIni.Equals(TEXT("false"), ESearchCase::IgnoreCase);
	}
	return bActiva;
}

void UAlsasuaTelemetria::Latido()
{
	UWorld* W = GetWorld();
	if (!W)
	{
		return;
	}

	static double UltimaMarca = FPlatformTime::Seconds();
	const double Ahora = FPlatformTime::Seconds();
	const double Tramo = Ahora - UltimaMarca;
	UltimaMarca = Ahora;
	if (Tramo <= 0.0 || FramesDesdeLatido <= 0)
	{
		return;
	}
	const float Fps = (float)(FramesDesdeLatido / Tramo);
	const float Ms = (float)(1000.0 * Tramo / FramesDesdeLatido);
	FramesDesdeLatido = 0;

	const FPlatformMemoryStats Mem = FPlatformMemory::GetStats();
	const int32 MbFisica = (int32)(Mem.UsedPhysical / (1024 * 1024));

	if (!Socket)
	{
		Socket = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->CreateSocket(
			NAME_DGram, TEXT("AlsasuaTelemetria"), /*bForceUDP*/true);
		Destino = ISocketSubsystem::Get(PLATFORM_SOCKETSUBSYSTEM)->
			CreateInternetAddr();
		bool bIpValida = false;
		Destino->SetIp(TEXT("127.0.0.1"), bIpValida);
		Destino->SetPort((int32)Puerto);
	}

	const FString Linea = FString::Printf(
		TEXT("{\"t\":%.1f,\"fps\":%.1f,\"ms\":%.1f,\"mb\":%d}\n"),
		W->GetTimeSeconds(), Fps, Ms, MbFisica);
	FTCHARToUTF8 Utf8(*Linea);
	int32 Enviados = 0;
	Socket->SendTo((const uint8*)Utf8.Get(), Utf8.Length(), Enviados, *Destino);
}
