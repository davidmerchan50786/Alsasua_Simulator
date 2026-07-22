// DialogoSubsystem.cpp
#include "DialogoSubsystem.h"
#include "ApoyoPopularSubsystem.h"

void UDialogoSubsystem::Iniciar(UConversacionDialogo* Conv)
{
	if (!Conv || Conv->Nodos.Num() == 0) return;
	Conversacion = Conv;
	IrA(Conv->Inicio.IsNone() ? Conv->Nodos[0].Id : Conv->Inicio);
}

TArray<FString> UDialogoSubsystem::OpcionesActuales() const
{
	TArray<FString> R;
	if (Actual) for (const FOpcionDialogo& O : Actual->Opciones) R.Add(O.Texto);
	return R;
}

void UDialogoSubsystem::IrA(FName Id)
{
	Actual = Conversacion ? Conversacion->BuscarNodo(Id) : nullptr;
	if (!Actual) { Terminar(); return; }
	OnNodoMostrado.Broadcast(Actual->Hablante, Actual->Texto, Actual->Opciones.Num() > 0);
}

void UDialogoSubsystem::Elegir(int32 Indice)
{
	if (!Actual) return;

	if (Actual->Opciones.Num() > 0)
	{
		if (!Actual->Opciones.IsValidIndex(Indice)) return;
		const FOpcionDialogo& Op = Actual->Opciones[Indice];

		// efecto sobre el apoyo popular
		if (!FMath::IsNearlyZero(Op.DeltaApoyo))
			if (UGameInstance* GI = GetGameInstance())
				if (UApoyoPopularSubsystem* Ap = GI->GetSubsystem<UApoyoPopularSubsystem>())
				{
					if (Op.DeltaApoyo > 0) Ap->SumarApoyo(Op.DeltaApoyo, TEXT("dialogo"));
					else                   Ap->RestarApoyo(-Op.DeltaApoyo, TEXT("dialogo"));
				}

		if (Op.Destino.IsNone()) Terminar();
		else IrA(Op.Destino);
	}
	else
	{
		// línea automática: avanza por Auto (o termina).
		if (Actual->Auto.IsNone()) Terminar();
		else IrA(Actual->Auto);
	}
}

void UDialogoSubsystem::Terminar()
{
	if (Actual || Conversacion)
	{
		Actual = nullptr;
		Conversacion = nullptr;
		OnDialogoFin.Broadcast();
	}
}
