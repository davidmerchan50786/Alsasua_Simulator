#pragma once
#include "CoreMinimal.h"
#include "Delegates/Delegate.h"
#include "Systems/Dialog/DialogTypes.h"
#include "DialogInstance.generated.h"

UCLASS(BlueprintType)
class GF_DIALOGOS_API UDialogInstance : public UObject {
    GENERATED_BODY()
public:
    void Init(class UDialogAsset* Asset, AActor* InParticipant);
    void Shutdown();

    UFUNCTION(BlueprintCallable, Category="AAA|Dialog")
    FDialogNode GetCurrentNode() const;

    UFUNCTION(BlueprintCallable, Category="AAA|Dialog")
    void SelectOption(int32 OptionIndex);

    UPROPERTY(BlueprintReadWrite, Category="AAA|Dialog")
    float SkillModifier = 0.f;

    DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FOnDialogNodeReached, FDialogNode, Node);
    UPROPERTY(BlueprintAssignable)
    FOnDialogNodeReached OnNodeReached;

private:
    UPROPERTY()
    class UDialogAsset* SourceAsset;

    UPROPERTY()
    AActor* Participant;

    int32 CurrentNodeID = 0;

    FTimerHandle AutoAdvanceTimerHandle;
    void AutoAdvanceTick();
};
