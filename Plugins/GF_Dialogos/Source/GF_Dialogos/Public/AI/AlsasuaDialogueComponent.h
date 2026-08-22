#pragma once

#include "CoreMinimal.h"
#include "Components/ActorComponent.h"
#include "Delegates/Delegate.h"
#include "AlsasuaDialogueComponent.generated.h"

DECLARE_DYNAMIC_MULTICAST_DELEGATE_TwoParams(FOnDialogueTriggered, FString, SpeakerName, FString, Line);

USTRUCT(BlueprintType)
struct FDialogueMoodPool
{
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FString> Lines;
};

/** Sistema de diálogo reactivo para manifestantes */
UCLASS(ClassGroup=(Custom), meta=(BlueprintSpawnableComponent))
class GF_DIALOGOS_API UAlsasuaDialogueComponent : public UActorComponent
{
    GENERATED_BODY()

public:
    UAlsasuaDialogueComponent();

    // Pool de diálogos basados en el estado emocional del manifestante
    UPROPERTY(EditAnywhere, Category = "AAA|Dialog")
    TMap<int32, FDialogueMoodPool> DialogueByMood;

    // Emite una frase de acuerdo al estado actual
    UFUNCTION(BlueprintCallable, Category = "AAA|Dialog")
    void Speak();

    // Delegado para mostrar subtítulos en HUD
    UPROPERTY(BlueprintAssignable, Category = "AAA|Dialog")
    FOnDialogueTriggered OnSpoke;

protected:
    virtual void BeginPlay() override;
};
