#pragma once

#include "CoreMinimal.h"

// Log Categories AAA
DECLARE_LOG_CATEGORY_EXTERN(LogAlsasua, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAlsasuaGAS, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAlsasuaAI, Log, All);
DECLARE_LOG_CATEGORY_EXTERN(LogAlsasuaWorld, Log, All);

// Macros de validación Custom
#define ALSASUA_VCHECK(Pointer) if(!Pointer) { UE_LOG(LogAlsasua, Error, TEXT("[%s] Null Pointer: %s"), *FString(__FUNCTION__), TEXT(#Pointer)); return; }
#define ALSASUA_VCHECK_RET(Pointer, RetVal) if(!Pointer) { UE_LOG(LogAlsasua, Error, TEXT("[%s] Null Pointer: %s"), *FString(__FUNCTION__), TEXT(#Pointer)); return RetVal; }
