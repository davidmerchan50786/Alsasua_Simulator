#include "Systems/Dialog/DialogSubsystem.h"
#include "Systems/Dialog/DialogInstance.h"
#include "Systems/Dialog/DialogAsset.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/FileHelper.h"

UDialogInstance* UDialogSubsystem::StartDialog(AActor* Instigator, UDialogAsset* DialogContent)
{
	if (!DialogContent) return nullptr;

	UDialogInstance* NewInstance = NewObject<UDialogInstance>(this);
	NewInstance->Init(DialogContent, Instigator);
	return NewInstance;
}

bool UDialogSubsystem::LoadDialogFromJson(const FString& JsonPath, UDialogAsset* TargetAsset)
{
	if (!TargetAsset)
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadDialogFromJson: TargetAsset es nullptr."));
		return false;
	}

	FString JsonContent;
	if (!FFileHelper::LoadFileToString(JsonContent, *JsonPath))
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadDialogFromJson: No se pudo leer %s"), *JsonPath);
		return false;
	}

	TSharedPtr<FJsonObject> RootObject;
	const TSharedRef<TJsonReader<>> Reader = TJsonReaderFactory<>::Create(JsonContent);

	if (!FJsonSerializer::Deserialize(Reader, RootObject) || !RootObject.IsValid())
	{
		UE_LOG(LogTemp, Warning, TEXT("LoadDialogFromJson: JSON inválido en %s"), *JsonPath);
		return false;
	}

	// Leer texto descriptivo.
	if (RootObject->HasField(TEXT("DialogText")))
	{
		TargetAsset->DialogText = FText::FromString(RootObject->GetStringField(TEXT("DialogText")));
	}

	// Leer nodo inicial.
	if (RootObject->HasField(TEXT("StartNodeID")))
	{
		TargetAsset->StartNodeID = RootObject->GetIntegerField(TEXT("StartNodeID"));
	}

	// Leer nodos.
	TargetAsset->Nodes.Empty();

	const TArray<TSharedPtr<FJsonValue>>* NodesArray;
	if (RootObject->TryGetArrayField(TEXT("Nodes"), NodesArray))
	{
		for (const TSharedPtr<FJsonValue>& NodeValue : *NodesArray)
		{
			const TSharedPtr<FJsonObject>& NodeObj = NodeValue->AsObject();
			if (!NodeObj.IsValid()) continue;

			FDialogNode Node;
			Node.ID = NodeObj->GetIntegerField(TEXT("ID"));
			Node.DialogueText = FText::FromString(NodeObj->GetStringField(TEXT("DialogueText")));

			// Tipo de nodo.
			const FString TypeStr = NodeObj->GetStringField(TEXT("Type"));
			if (TypeStr == TEXT("NPC_Statement"))        Node.Type = EDialogNodeType::NPC_Statement;
			else if (TypeStr == TEXT("Player_Choice"))    Node.Type = EDialogNodeType::Player_Choice;
			else if (TypeStr == TEXT("Condition_Branch")) Node.Type = EDialogNodeType::Condition_Branch;
			else if (TypeStr == TEXT("Action_Trigger"))   Node.Type = EDialogNodeType::Action_Trigger;
			else if (TypeStr == TEXT("End_Conversation")) Node.Type = EDialogNodeType::End_Conversation;

			// Opciones.
			const TArray<TSharedPtr<FJsonValue>>* OptionsArray;
			if (NodeObj->TryGetArrayField(TEXT("Options"), OptionsArray))
			{
				for (const TSharedPtr<FJsonValue>& OptValue : *OptionsArray)
				{
					const TSharedPtr<FJsonObject>& OptObj = OptValue->AsObject();
					if (!OptObj.IsValid()) continue;

					FDialogOption Opt;
					Opt.OptionText = FText::FromString(OptObj->GetStringField(TEXT("OptionText")));
					Opt.TargetNodeID = OptObj->GetIntegerField(TEXT("TargetNodeID"));
					Opt.bRequiresSkillCheck = OptObj->GetBoolField(TEXT("bRequiresSkillCheck"));
					Opt.DifficultyClass = OptObj->GetIntegerField(TEXT("DifficultyClass"));
					Node.Options.Add(Opt);
				}
			}

			TargetAsset->Nodes.Add(Node);
		}
	}

	UE_LOG(LogTemp, Log, TEXT("LoadDialogFromJson: Cargados %d nodos desde %s"),
		TargetAsset->Nodes.Num(), *JsonPath);

	return true;
}
