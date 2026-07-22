#pragma once
#include "CoreMinimal.h"
#include "EditorUtilityActor.h"
#include "AlsasuaEditorTool.generated.h"

UCLASS()
class ALSASUAEDITOR_API AAlsasuaEditorTool : public AEditorUtilityActor
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category = "Alsasua|Editor")
    void DebugBuildings();
};
