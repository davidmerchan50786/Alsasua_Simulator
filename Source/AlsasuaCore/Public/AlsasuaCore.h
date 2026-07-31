#pragma once

#include "CoreMinimal.h"

// Log Categories AAA — type definitions + exported extern declarations
struct ALSASUACORE_API FLogCategoryLogAlsasua : public FLogCategory<ELogVerbosity::Log, ELogVerbosity::All>
{
    FORCEINLINE FLogCategoryLogAlsasua() : FLogCategory(TEXT("LogAlsasua")) {}
};
ALSASUACORE_API extern FLogCategoryLogAlsasua LogAlsasua;

struct ALSASUACORE_API FLogCategoryLogAlsasuaGAS : public FLogCategory<ELogVerbosity::Log, ELogVerbosity::All>
{
    FORCEINLINE FLogCategoryLogAlsasuaGAS() : FLogCategory(TEXT("LogAlsasuaGAS")) {}
};
ALSASUACORE_API extern FLogCategoryLogAlsasuaGAS LogAlsasuaGAS;

struct ALSASUACORE_API FLogCategoryLogAlsasuaAI : public FLogCategory<ELogVerbosity::Log, ELogVerbosity::All>
{
    FORCEINLINE FLogCategoryLogAlsasuaAI() : FLogCategory(TEXT("LogAlsasuaAI")) {}
};
ALSASUACORE_API extern FLogCategoryLogAlsasuaAI LogAlsasuaAI;

struct ALSASUACORE_API FLogCategoryLogAlsasuaWorld : public FLogCategory<ELogVerbosity::Log, ELogVerbosity::All>
{
    FORCEINLINE FLogCategoryLogAlsasuaWorld() : FLogCategory(TEXT("LogAlsasuaWorld")) {}
};
ALSASUACORE_API extern FLogCategoryLogAlsasuaWorld LogAlsasuaWorld;

// Macros de validación Custom
#define ALSASUA_VCHECK(Pointer) if(!Pointer) { UE_LOG(LogAlsasua, Error, TEXT("[%s] Null Pointer: %s"), *FString(__FUNCTION__), TEXT(#Pointer)); return; }
#define ALSASUA_VCHECK_RET(Pointer, RetVal) if(!Pointer) { UE_LOG(LogAlsasua, Error, TEXT("[%s] Null Pointer: %s"), *FString(__FUNCTION__), TEXT(#Pointer)); return RetVal; }
