#pragma once
#include "Stats/Stats.h"

DECLARE_STATS_GROUP(TEXT("Alsasua|Crowd"), STATGROUP_AlsasuaCrowd, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Alsasua|AI"), STATGROUP_AlsasuaAI, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Alsasua|World"), STATGROUP_AlsasuaWorld, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Alsasua|GAS"), STATGROUP_AlsasuaGAS, STATCAT_Advanced);
DECLARE_STATS_GROUP(TEXT("Alsasua|Audio"), STATGROUP_AlsasuaAudio, STATCAT_Advanced);

DECLARE_CYCLE_STAT(TEXT("MassParallel Tick"), STAT_AlsasuaCrowd_MassParallelTick, STATGROUP_AlsasuaCrowd);
DECLARE_CYCLE_STAT(TEXT("ParallelFor Update"), STAT_AlsasuaCrowd_ParallelFor, STATGROUP_AlsasuaCrowd);
DECLARE_CYCLE_STAT(TEXT("TacticManager Tick"), STAT_AlsasuaAI_TacticTick, STATGROUP_AlsasuaAI);
DECLARE_CYCLE_STAT(TEXT("CrowdSentiment Tick"), STAT_AlsasuaAI_CrowdSentimentTick, STATGROUP_AlsasuaAI);
DECLARE_CYCLE_STAT(TEXT("Weather Tick"), STAT_AlsasuaWorld_WeatherTick, STATGROUP_AlsasuaWorld);
DECLARE_CYCLE_STAT(TEXT("Audio Occlusion Trace"), STAT_AlsasuaAudio_OcclusionTrace, STATGROUP_AlsasuaAudio);
DECLARE_CYCLE_STAT(TEXT("Footstep Lookup"), STAT_AlsasuaWorld_FootstepLookup, STATGROUP_AlsasuaWorld);
