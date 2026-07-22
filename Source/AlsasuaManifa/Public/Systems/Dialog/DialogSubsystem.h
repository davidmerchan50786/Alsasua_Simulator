#pragma once
#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "DialogSubsystem.generated.h"

UCLASS()
class ALSASUAMANIFA_API UDialogSubsystem : public UWorldSubsystem {
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable, Category="AAA|Dialog")
    class UDialogInstance* StartDialog(AActor* Instigator, class UDialogAsset* DialogContent);

    // Helper para cargar diálogos desde JSON en Runtime
    UFUNCTION(BlueprintCallable, Category="AAA|Dialog")
    bool LoadDialogFromJson(const FString& JsonPath, class UDialogAsset* TargetAsset);
};