// DialogoSubsystem.cpp
#include "DialogoSubsystem.h"
#include "ApoyoPopularSubsystem.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

namespace
{
	/** Los nodos del JSON se identifican por entero; aquí van por FName. */
	FName NombreNodo(int32 Id)
	{
		return Id >= 0 ? FName(*FString::FromInt(Id)) : NAME_None;
	}
}

UConversacionDialogo* UDialogoSubsystem::CargarConversacion(const FString& NombreNPC)
{
	if (NombreNPC.IsEmpty()) return nullptr;
	if (TObjectPtr<UConversacionDialogo>* Ya = Cache.Find(NombreNPC)) return *Ya;

	const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(),
		TEXT("Dialogs"), NombreNPC + TEXT(".json"));
	FString Texto;
	if (!FFileHelper::LoadFileToString(Texto, *Ruta))
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogo] no existe %s"), *Ruta);
		return nullptr;
	}

	TSharedPtr<FJsonObject> Doc;
	const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
	if (!FJsonSerializer::Deserialize(R, Doc) || !Doc.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogo] %s no es un objeto JSON"), *Ruta);
		return nullptr;
	}

	const TArray<TSharedPtr<FJsonValue>>* Nodos = nullptr;
	if (!Doc->TryGetArrayField(TEXT("Nodes"), Nodos) || !Nodos || Nodos->Num() == 0)
	{
		UE_LOG(LogTemp, Warning, TEXT("[Dialogo] %s sin nodos"), *Ruta);
		return nullptr;
	}

	UConversacionDialogo* Conv = NewObject<UConversacionDialogo>(this);
	Conv->Inicio = NombreNodo(Doc->HasField(TEXT("StartNodeID"))
		? (int32)Doc->GetNumberField(TEXT("StartNodeID")) : 0);

	int32 ConChequeo = 0;
	for (const TSharedPtr<FJsonValue>& V : *Nodos)
	{
		const TSharedPtr<FJsonObject> O = V->AsObject();
		if (!O.IsValid() || !O->HasField(TEXT("ID"))) continue;

		FNodoDialogo N;
		N.Id = NombreNodo((int32)O->GetNumberField(TEXT("ID")));
		// El fichero es de un NPC entero, así que el hablante es su nombre. El
		// JSON no lo trae por nodo, y no hace falta: no hay diálogos a tres.
		N.Hablante = NombreNPC;
		N.Texto = O->HasField(TEXT("DialogueText")) ? O->GetStringField(TEXT("DialogueText")) : FString();

		const TArray<TSharedPtr<FJsonValue>>* Opciones = nullptr;
		if (O->TryGetArrayField(TEXT("Options"), Opciones) && Opciones)
		{
			for (const TSharedPtr<FJsonValue>& OV : *Opciones)
			{
				const TSharedPtr<FJsonObject> OO = OV->AsObject();
				if (!OO.IsValid()) continue;

				FOpcionDialogo Op;
				Op.Texto = OO->HasField(TEXT("OptionText")) ? OO->GetStringField(TEXT("OptionText")) : FString();
				Op.Destino = NombreNodo(OO->HasField(TEXT("TargetNodeID"))
					? (int32)OO->GetNumberField(TEXT("TargetNodeID")) : -1);

				bool bChequeo = false;
				if (OO->TryGetBoolField(TEXT("bRequiresSkillCheck"), bChequeo) && bChequeo) ++ConChequeo;

				N.Opciones.Add(Op);
			}
		}

		// End_Conversation cierra: ni opciones ni línea automática. Los demás
		// tipos, si no tienen opciones, encadenan con el nodo siguiente por ID,
		// que es como está escrito el contenido.
		const FString Tipo = O->HasField(TEXT("Type")) ? O->GetStringField(TEXT("Type")) : FString();
		if (N.Opciones.Num() == 0 && Tipo != TEXT("End_Conversation"))
		{
			N.Auto = NombreNodo((int32)O->GetNumberField(TEXT("ID")) + 1);
		}

		Conv->Nodos.Add(MoveTemp(N));
	}

	if (Conv->Nodos.Num() == 0) return nullptr;

	// Los enlaces a nodos que no existen se dejan en fin de conversación: mejor
	// cerrar que quedarse colgado en un nodo vacío.
	int32 Rotos = 0;
	for (FNodoDialogo& N : Conv->Nodos)
	{
		if (!N.Auto.IsNone() && !Conv->BuscarNodo(N.Auto)) { N.Auto = NAME_None; ++Rotos; }
		for (FOpcionDialogo& Op : N.Opciones)
		{
			if (!Op.Destino.IsNone() && !Conv->BuscarNodo(Op.Destino)) { Op.Destino = NAME_None; ++Rotos; }
		}
	}

	// bRequiresSkillCheck y DifficultyClass están en el dato y NO se aplican:
	// FOpcionDialogo no tiene con qué, y gatear opciones por una tirada es una
	// decisión de diseño, no de un cargador. Se dice cuántas para que el hueco
	// se vea en el log en vez de desaparecer.
	UE_LOG(LogTemp, Log,
		TEXT("[Dialogo] %s: %d nodos (%d enlaces rotos cerrados, %d opciones con tirada sin aplicar)."),
		*NombreNPC, Conv->Nodos.Num(), Rotos, ConChequeo);

	Cache.Add(NombreNPC, Conv);
	return Conv;
}

bool UDialogoSubsystem::IniciarConNPC(const FString& NombreNPC)
{
	UConversacionDialogo* Conv = CargarConversacion(NombreNPC);
	if (!Conv) return false;
	Iniciar(Conv);
	return EnCurso();
}

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
