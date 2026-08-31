#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "Politics/FactionSubsystem.h"
#include "Systems/Dialog/DialogInstance.h"
#include "Systems/Dialog/DialogAsset.h"
#include "Systems/Dialog/DialogTypes.h"
#include "MisionTipos.h"

#if WITH_AUTOMATION_WORKER

namespace AlsasuaTests
{
    // ── FactionSubsystem Reputation Tests ────────────────────────────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactionReputationTest,
        "Alsasua.FactionSubsystem.Reputation",
        EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

    bool FFactionReputationTest::RunTest(const FString& Parameters)
    {
        AddWarning(TEXT("Skipped: requires live world context"));
        return true;
    }

    // ── Dialog System Tests ─────────────────────────────────────────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogInstanceBasicTest,
        "Alsasua.Dialog.InstanceBasic",
        EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

    bool FDialogInstanceBasicTest::RunTest(const FString& Parameters)
    {
        UDialogAsset* Asset = NewObject<UDialogAsset>();

        // Create a simple 2-node dialog.
        FDialogNode Node0;
        Node0.ID = 0;
        Node0.Type = EDialogNodeType::NPC_Statement;
        Node0.DialogueText = FText::FromString(TEXT("Hola, ¿qué tal?"));

        FDialogOption Opt0;
        Opt0.OptionText = FText::FromString(TEXT("Bien"));
        Opt0.TargetNodeID = 1;
        Opt0.bRequiresSkillCheck = false;
        Node0.Options.Add(Opt0);

        FDialogNode Node1;
        Node1.ID = 1;
        Node1.Type = EDialogNodeType::End_Conversation;
        Node1.DialogueText = FText::FromString(TEXT("Adiós"));

        Asset->Nodes.Add(Node0);
        Asset->Nodes.Add(Node1);
        Asset->StartNodeID = 0;

        UDialogInstance* Inst = NewObject<UDialogInstance>();
        Inst->Init(Asset, nullptr);

        FDialogNode Current = Inst->GetCurrentNode();
        TestEqual(TEXT("Start node ID is 0"), Current.ID, 0);
        TestEqual(TEXT("Start node has 1 option"), Current.Options.Num(), 1);

        // Select option 0 → should advance to node 1.
        Inst->SelectOption(0);
        Current = Inst->GetCurrentNode();
        TestEqual(TEXT("After select, node ID is 1"), Current.ID, 1);

        return true;
    }

    // ── MisionDef Tests (esquema vivo UMisionDef/FObjetivoMision) ────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMisionDefTest,
        "Alsasua.Mision.DefStructure",
        EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

    bool FMisionDefTest::RunTest(const FString& Parameters)
    {
        UMisionDef* Def = NewObject<UMisionDef>();
        Def->Id = FName("M00");
        Def->Titulo = TEXT("Esnatu, Altsasu");
        Def->Dificultad = 1;
        Def->RecompensaDinero = 500;
        Def->RecompensaApoyo = 10.f;

        FObjetivoMision Obj0;
        Obj0.Id = FName("mover");
        Obj0.Descripcion = TEXT("Muévete 5 metros");
        Obj0.Meta = 1;
        Def->Objetivos.Add(Obj0);

        FObjetivoMision Obj1;
        Obj1.Id = FName("plaza");
        Obj1.Descripcion = TEXT("Llega a la plaza");
        Obj1.Meta = 1;
        Def->Objetivos.Add(Obj1);

        TestEqual(TEXT("Mission has 2 objectives"), Def->Objetivos.Num(), 2);
        TestEqual(TEXT("Mission reward is 500"), Def->RecompensaDinero, 500);
        TestFalse(TEXT("Obj0 not completed"), Def->Objetivos[0].Completado());

        Def->Objetivos[0].Progreso = 1;
        TestTrue(TEXT("Obj0 now completed"), Def->Objetivos[0].Completado());
        TestFalse(TEXT("Obj1 still not completed"), Def->Objetivos[1].Completado());

        return true;
    }

    // ── Math / Clamp Logic Tests ────────────────────────────────────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApoyoPopularDecayTest,
        "Alsasua.Economy.ClampLogic",
        EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

    bool FApoyoPopularDecayTest::RunTest(const FString& Parameters)
    {
        float Apoyo = 50.f;
        Apoyo = FMath::Clamp(Apoyo + 30.f, 0.f, 100.f);
        TestEqual(TEXT("Apoyo clamped at 80"), Apoyo, 80.f);

        Apoyo = FMath::Clamp(Apoyo - 200.f, 0.f, 100.f);
        TestEqual(TEXT("Apoyo clamped at 0"), Apoyo, 0.f);

        float Tension = 0.8f;
        Tension = FMath::Max(0.0f, Tension - (0.1f * 2.0f));
        TestTrue(TEXT("Tension decayed"), Tension < 0.8f);

        return true;
    }

    // ── FindNodeByID Tests ──────────────────────────────────────────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogAssetFindNodeTest,
        "Alsasua.Dialog.FindNodeByID",
        EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::ProductFilter)

    bool FDialogAssetFindNodeTest::RunTest(const FString& Parameters)
    {
        UDialogAsset* Asset = NewObject<UDialogAsset>();

        FDialogNode Node0;
        Node0.ID = 0;
        Node0.Type = EDialogNodeType::NPC_Statement;
        Node0.DialogueText = FText::FromString(TEXT("Test"));
        Asset->Nodes.Add(Node0);

        const FDialogNode* Found = Asset->FindNodeByID(0);
        TestNotNull(TEXT("Node 0 found"), Found);
        if (Found)
        {
            TestEqual(TEXT("Node 0 text matches"), Found->DialogueText.ToString(), FString("Test"));
        }

        const FDialogNode* NotFound = Asset->FindNodeByID(99);
        TestNull(TEXT("Node 99 not found"), NotFound);

        return true;
    }
}

#endif
