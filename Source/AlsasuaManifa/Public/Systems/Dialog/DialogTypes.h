#pragma once

#include "CoreMinimal.h"
#include "GameplayTagContainer.h"
#include "DialogTypes.generated.h"

// Tipos de nodos en el grafo de conversación
UENUM(BlueprintType)
enum class EDialogNodeType : uint8 {
    NPC_Statement,    // El NPC habla
    Player_Choice,    // El jugador elige entre varias opciones
    Condition_Branch, // Nodo invisible que bifurca según condiciones
    Action_Trigger,   // Nodo que ejecuta una lógica y salta al siguiente
    End_Conversation  // Finaliza el diálogo
};

// Tipos de condiciones (Basado en el éxito de Baldur's Gate 3)
UENUM(BlueprintType)
enum class EDialogConditionType : uint8 {
    AttributeCheck,   // Fuerza, Stamina, PopularSupport, etc.
    KarmaCheck,       // Si eres pacífico o violento
    GameplayTagCheck, // Si tienes ciertos tags (ej: "HasVistoAlAlcalde")
    RandomRoll,       // Tirada de dados (Persuasión, Intimidación)
    ItemCheck         // Si tienes un megáfono, pruebas, etc.
};

// Estructura de condición individual
USTRUCT(BlueprintType)
struct FDialogCondition {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDialogConditionType Type = EDialogConditionType::GameplayTagCheck;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FGameplayTag RequiredTag;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    float RequiredValue = 0.f;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bNegate = false; // "Si NO tiene el tag..."
};

// Estructura de opción de diálogo (Aristas del grafo)
USTRUCT(BlueprintType)
struct FDialogOption {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText OptionText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 TargetNodeID = -1;

    // Condiciones para que esta opción aparezca (Ej: [INTIMIDACION])
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FDialogCondition> VisibilityConditions;

    // Si la opción requiere una tirada de dados
    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    bool bRequiresSkillCheck = false;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 DifficultyClass = 10;
};

// Nodo principal del diálogo
USTRUCT(BlueprintType)
struct FDialogNode {
    GENERATED_BODY()

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    int32 ID = 0;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    EDialogNodeType Type = EDialogNodeType::NPC_Statement;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FText DialogueText;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TSoftObjectPtr<USoundBase> VoiceOver;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    TArray<FDialogOption> Options;

    UPROPERTY(EditAnywhere, BlueprintReadWrite)
    FName NextNodeIDIfNotChoice = NAME_None;
};