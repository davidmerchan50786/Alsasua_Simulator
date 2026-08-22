#pragma once
#include "CoreMinimal.h"
#include "Engine/DataAsset.h"
#include "Systems/Dialog/DialogTypes.h"
#include "DialogAsset.generated.h"

/**
 * DataAsset que contiene un grafo de conversación completo.
 * Almacena una lista de nodos (FDialogNode) con IDs únicos.
 */
UCLASS(BlueprintType)
class GF_DIALOGOS_API UDialogAsset : public UDataAsset
{
    GENERATED_BODY()
public:
    /** Texto descriptivo del diálogo (para debug/editors). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
    FText DialogText;

    /** Nodo inicial del diálogo (por defecto, el primero de la lista). */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
    int32 StartNodeID = 0;

    /** Grafo completo de nodos de diálogo. */
    UPROPERTY(EditAnywhere, BlueprintReadWrite, Category = "Dialog")
    TArray<FDialogNode> Nodes;

    /** Buscar un nodo por ID (nullptr si no existe). */
    const FDialogNode* FindNodeByID(int32 NodeID) const
    {
        for (const FDialogNode& Node : Nodes)
        {
            if (Node.ID == NodeID) return &Node;
        }
        return nullptr;
    }
};
