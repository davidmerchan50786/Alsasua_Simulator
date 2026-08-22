#include "Politics/FactionSubsystem.h"
#include "Engine/World.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Serialization/JsonReader.h"
#include "Serialization/JsonSerializer.h"

void UFactionSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
    Super::Initialize(Collection);

    // Cargar desde JSON si existe; fallback a datos embebidos.
    if (!CargarDesdeJSON())
    {
        RegisterFaction({FName("Resistencia"),  TEXT("Resistencia"),  50.f, 0.f, {}});
        RegisterFaction({FName("ERTA"),         TEXT("ERTA"),         40.f, 0.f, {}});
        RegisterFaction({FName("Policia"),      TEXT("Policía Local"),60.f, 0.f, {}});
        RegisterFaction({FName("GuardiaCivil"), TEXT("Guardia Civil"),70.f, 0.f, {}});
        RegisterFaction({FName("DeepState"),    TEXT("Deep State"),   30.f, 0.f, {}});
        RegisterFaction({FName("Gremios"),      TEXT("Gremios"),      45.f, 0.f, {}});
        RegisterFaction({FName("Prensa"),       TEXT("Prensa"),       55.f, 0.f, {}});
        RegisterFaction({FName("Politicos"),    TEXT("Políticos"),    65.f, 0.f, {}});
        RegisterFaction({FName("ElCentro"),     TEXT("El Centro"),    50.f, 0.f, {}});
        RegisterFaction({FName("LaAsamblea"),   TEXT("La Asamblea"),  55.f, 0.f, {}});
        RegisterFaction({FName("ElGremio"),     TEXT("El Gremio"),    45.f, 0.f, {}});

        if (FFactionData* Res = Factions.Find(FName("Resistencia")))
        {
            Res->Relations.Add(FName("Policia"), -30.f);
            Res->Relations.Add(FName("GuardiaCivil"), -40.f);
            Res->Relations.Add(FName("DeepState"), -60.f);
            Res->Relations.Add(FName("Gremios"), 20.f);
        }
    }

    for (const auto& Pair : Factions)
    {
        FactionReputation.FindOrAdd(Pair.Key, 50.f);
    }
}

bool UFactionSubsystem::CargarDesdeJSON()
{
    const FString Ruta = FPaths::Combine(FPaths::ProjectContentDir(), TEXT("Datos/factions.json"));
    FString Texto;
    if (!FFileHelper::LoadFileToString(Texto, *Ruta)) return false;

    const TSharedRef<TJsonReader<>> R = TJsonReaderFactory<>::Create(Texto);
    TSharedPtr<FJsonValue> RootVal;
    if (!FJsonSerializer::Deserialize(R, RootVal)) return false;

    const auto& Root = RootVal->AsObject();
    if (!Root.IsValid()) return false;

    const TArray<TSharedPtr<FJsonValue>>* FactionsArr = nullptr;
    if (!Root->TryGetArrayField(TEXT("factions"), FactionsArr)) return false;

    for (const auto& Item : *FactionsArr)
    {
        const auto& O = Item->AsObject();
        if (!O.IsValid()) continue;
        FFactionData Data;
        Data.Id = FName(*O->GetStringField(TEXT("id")));
        Data.DisplayName = O->GetStringField(TEXT("nombre"));
        Data.Influence = O->GetNumberField(TEXT("influence"));
        Data.Suspicion = O->GetNumberField(TEXT("suspicion"));
        RegisterFaction(Data);
    }

    const TArray<TSharedPtr<FJsonValue>>* RelArr = nullptr;
    if (Root->TryGetArrayField(TEXT("relations"), RelArr))
    {
        for (const auto& Item : *RelArr)
        {
            const auto& O = Item->AsObject();
            if (!O.IsValid()) continue;
            const FName From(*O->GetStringField(TEXT("from")));
            const FName To(*O->GetStringField(TEXT("to")));
            const float Value = O->GetNumberField(TEXT("value"));
            if (FFactionData* F = Factions.Find(From))
            {
                F->Relations.Add(To, Value);
            }
        }
    }

    UE_LOG(LogTemp, Log, TEXT("[Facciones] %d facciones cargadas desde JSON"), Factions.Num());
    return true;
}

void UFactionSubsystem::RegisterFaction(const FFactionData& Data)
{
    Factions.Add(Data.Id, Data);
}

void UFactionSubsystem::RecordPoliticalEvent(FName SubjectFaction, FName TargetFaction, float Impact)
{
    if (FFactionData* Sub = Factions.Find(SubjectFaction))
    {
        Sub->Influence = FMath::Clamp(Sub->Influence + Impact, 0.f, 100.f);
    }
    if (FFactionData* Target = Factions.Find(TargetFaction))
    {
        Target->Suspicion = FMath::Clamp(Target->Suspicion + FMath::Abs(Impact)*0.5f, 0.f, 100.f);
    }
}

void UFactionSubsystem::PublishEvidence(FName TargetFaction, float Strength)
{
    if (FFactionData* F = Factions.Find(TargetFaction))
    {
        F->Influence = FMath::Clamp(F->Influence - Strength, 0.f, 100.f);
    }
}

FFactionData UFactionSubsystem::GetFactionData(FName Id) const
{
    if (const FFactionData* Found = Factions.Find(Id))
    {
        return *Found;
    }
    return FFactionData();
}

// ── Reputation System ──────────────────────────────────────────────────

float UFactionSubsystem::GetReputation(FName FactionId) const
{
    if (const float* Rep = FactionReputation.Find(FactionId))
    {
        return *Rep;
    }
    return 50.f; // Default neutral reputation.
}

void UFactionSubsystem::ModifyReputation(FName FactionId, float Delta)
{
    float& Rep = FactionReputation.FindOrAdd(FactionId, 50.f);
    Rep = FMath::Clamp(Rep + Delta, 0.f, 100.f);
}

bool UFactionSubsystem::AreAllied(FName FactionA, FName FactionB) const
{
    if (const FFactionData* DataA = Factions.Find(FactionA))
    {
        if (const float* Relation = DataA->Relations.Find(FactionB))
        {
            return *Relation >= 20.f; // Allied if positive relation ≥ 20.
        }
    }
    return false;
}
