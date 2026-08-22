#include "Systems/Dialog/DialogInstance.h"
#include "Systems/Dialog/DialogAsset.h"
#include "Kismet/GameplayStatics.h"
#include "Engine/World.h"
#include "Engine/TimerHandle.h"

static constexpr float AutoAdvanceDelaySeconds = 3.5f;

void UDialogInstance::Init(UDialogAsset* Asset, AActor* InParticipant)
{
	SourceAsset = Asset;
	Participant = InParticipant;
	CurrentNodeID = Asset ? Asset->StartNodeID : 0;

	const FDialogNode FirstNode = GetCurrentNode();
	if (FirstNode.ID != 0 || SourceAsset)
	{
		OnNodeReached.Broadcast(FirstNode);

		// Start auto-advance timer if the first node is an NPC monologue.
		if (FirstNode.Options.Num() == 0 && FirstNode.NextNodeIDIfNotChoice != NAME_None
			&& FirstNode.Type != EDialogNodeType::End_Conversation)
		{
			if (UWorld* World = InParticipant ? InParticipant->GetWorld() : nullptr)
			{
				World->GetTimerManager().SetTimer(
					AutoAdvanceTimerHandle, this, &UDialogInstance::AutoAdvanceTick,
					AutoAdvanceDelaySeconds, false);
			}
		}
	}
}

void UDialogInstance::Shutdown()
{
	if (Participant)
	{
		if (UWorld* World = Participant->GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
		}
	}
}

FDialogNode UDialogInstance::GetCurrentNode() const
{
	if (!SourceAsset)
	{
		return FDialogNode();
	}

	const FDialogNode* Node = SourceAsset->FindNodeByID(CurrentNodeID);
	if (Node)
	{
		return *Node;
	}

	return FDialogNode();
}

void UDialogInstance::SelectOption(int32 OptionIndex)
{
	// Clear any pending auto-advance when the player makes a choice.
	if (Participant)
	{
		if (UWorld* World = Participant->GetWorld())
		{
			World->GetTimerManager().ClearTimer(AutoAdvanceTimerHandle);
		}
	}

	const FDialogNode Current = GetCurrentNode();
	if (!Current.Options.IsValidIndex(OptionIndex))
	{
		return;
	}

	const FDialogOption& Selected = Current.Options[OptionIndex];

	// Skill check: roll d20 + modifier vs DC.
	if (Selected.bRequiresSkillCheck)
	{
		int32 Roll = FMath::RandRange(1, 20);
		int32 Total = Roll + FMath::RoundToInt(SkillModifier);

		if (Total < Selected.DifficultyClass)
		{
			OnNodeReached.Broadcast(Current);
			return;
		}
	}

	CurrentNodeID = Selected.TargetNodeID;

	const FDialogNode* NextNode = SourceAsset ? SourceAsset->FindNodeByID(CurrentNodeID) : nullptr;
	if (NextNode)
	{
		if (!NextNode->VoiceOver.IsNull())
		{
			if (USoundBase* VoiceSound = NextNode->VoiceOver.LoadSynchronous())
			{
				UGameplayStatics::PlaySound2D(Participant, VoiceSound, 0.9f);
			}
		}

		OnNodeReached.Broadcast(*NextNode);

		// Restart auto-advance if next node is NPC monologue.
		if (NextNode->Type != EDialogNodeType::End_Conversation
			&& NextNode->Options.Num() == 0 && NextNode->NextNodeIDIfNotChoice != NAME_None)
		{
			if (UWorld* World = Participant ? Participant->GetWorld() : nullptr)
			{
				World->GetTimerManager().SetTimer(
					AutoAdvanceTimerHandle, this, &UDialogInstance::AutoAdvanceTick,
					AutoAdvanceDelaySeconds, false);
			}
		}
	}
}

void UDialogInstance::AutoAdvanceTick()
{
	const FDialogNode Current = GetCurrentNode();

	if (Current.Type == EDialogNodeType::End_Conversation)
	{
		return;
	}

	if (Current.Options.Num() == 0 && Current.NextNodeIDIfNotChoice != NAME_None)
	{
		// Advance directly to the next node.
		CurrentNodeID = FCString::Atoi(*Current.NextNodeIDIfNotChoice.ToString());

		const FDialogNode* NextNode = SourceAsset ? SourceAsset->FindNodeByID(CurrentNodeID) : nullptr;
		if (NextNode)
		{
			if (!NextNode->VoiceOver.IsNull())
			{
				if (USoundBase* VoiceSound = NextNode->VoiceOver.LoadSynchronous())
				{
					UGameplayStatics::PlaySound2D(Participant, VoiceSound, 0.9f);
				}
			}

			OnNodeReached.Broadcast(*NextNode);

			// Continue auto-advance if the next node is also NPC monologue.
			if (NextNode->Options.Num() == 0 && NextNode->NextNodeIDIfNotChoice != NAME_None
				&& NextNode->Type != EDialogNodeType::End_Conversation)
			{
				if (UWorld* World = Participant ? Participant->GetWorld() : nullptr)
				{
					World->GetTimerManager().SetTimer(
						AutoAdvanceTimerHandle, this, &UDialogInstance::AutoAdvanceTick,
						AutoAdvanceDelaySeconds, false);
				}
			}
		}
	}
}
