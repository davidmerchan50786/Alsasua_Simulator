#include "Misc/AutomationTest.h"
#include "Tests/AutomationCommon.h"
#include "Engine/World.h"
#include "AlsasuaManifa/Public/Politics/FactionSubsystem.h"
#include "AlsasuaManifa/Public/Systems/Dialog/DialogInstance.h"
#include "AlsasuaManifa/Public/Systems/Dialog/DialogAsset.h"
#include "AlsasuaManifa/Public/Systems/Dialog/DialogTypes.h"
#include "AlsasuaManifa/Public/Systems/MisionData.h"

#if WITH_AUTOMATION_WORKER

namespace AlsasuaTests
{
    // ── FactionSubsystem Reputation Tests ────────────────────────────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FFactionReputationTest,
        "Alsasua.FactionSubsystem.Reputation",
        EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

    bool FFactionReputationTest::RunTest(const FString& Parameters)
    {
        UWorld* World = FAutomationEditorCommonUtils::CreateNewMap();
        UFactionSubsystem* FactionSS = World->GetSubsystem<UFactionSubsystem>();
        if (!FactionSS)
        {
            AddError(TEXT("FactionSubsystem not found"));
            return true;
        }

        // Default reputation should be 50.
        float Rep = FactionSS->GetReputation(FName("Resistencia"));
        TestEqual(TEXT("Default reputation is 50"), Rep, 50.f);

        // Modify reputation.
        FactionSS->ModifyReputation(FName("Resistencia"), 20.f);
        Rep = FactionSS->GetReputation(FName("Resistencia"));
        TestEqual(TEXT("Reputation after +20 is 70"), Rep, 70.f);

        // Clamp at 0.
        FactionSS->ModifyReputation(FName("Resistencia"), -200.f);
        Rep = FactionSS->GetReputation(FName("Resistencia"));
        TestEqual(TEXT("Reputation clamped to 0"), Rep, 0.f);

        // Clamp at 100.
        FactionSS->ModifyReputation(FName("Resistencia"), 200.f);
        Rep = FactionSS->GetReputation(FName("Resistencia"));
        TestEqual(TEXT("Reputation clamped to 100"), Rep, 100.f);

        // AreAllied test.
        bool Allied = FactionSS->AreAllied(FName("Resistencia"), FName("Gremios"));
        TestTrue(TEXT("Resistencia-Gremios are allied"), Allied);

        bool Enemies = FactionSS->AreAllied(FName("Resistencia"), FName("Policia"));
        TestFalse(TEXT("Resistencia-Policia are not allied"), Enemies);

        return true;
    }

    // ── Dialog System Tests ─────────────────────────────────────────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FDialogInstanceBasicTest,
        "Alsasua.Dialog.InstanceBasic",
        EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

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

    // ── MisionData Tests ────────────────────────────────────────────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FMisionDataTest,
        "Alsasua.Mision.DataStructure",
        EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

    bool FMisionDataTest::RunTest(const FString& Parameters)
    {
        FMissionData Mission;
        Mission.MissionID = FName("M00");
        Mission.MissionName = FText::FromString(TEXT("Tutorial"));
        Mission.DifficultyLevel = 1;
        Mission.Reward.Money = 500.f;
        Mission.Reward.PopularSupport = 10.f;

        FMisionObjective Obj0;
        Obj0.ObjectiveID = FName("Obj_Move");
        Obj0.ObjectiveText = FText::FromString(TEXT("Muévete 5 metros"));
        Mission.Objectives.Add(Obj0);

        FMisionObjective Obj1;
        Obj1.ObjectiveID = FName("Obj_Plaza");
        Obj1.ObjectiveText = FText::FromString(TEXT("Llega a la plaza"));
        Mission.Objectives.Add(Obj1);

        TestEqual(TEXT("Mission has 2 objectives"), Mission.Objectives.Num(), 2);
        TestEqual(TEXT("Mission reward is 500"), Mission.Reward.Money, 500.f);
        TestFalse(TEXT("Obj0 not completed"), Mission.Objectives[0].bCompleted);

        Mission.Objectives[0].bCompleted = true;
        TestTrue(TEXT("Obj0 now completed"), Mission.Objectives[0].bCompleted);
        TestFalse(TEXT("Obj1 still not completed"), Mission.Objectives[1].bCompleted);

        return true;
    }

    // ── Math / Clamp Logic Tests ────────────────────────────────────────

    IMPLEMENT_SIMPLE_AUTOMATION_TEST(FApoyoPopularDecayTest,
        "Alsasua.Economy.ClampLogic",
        EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

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
        EAutomationTestFlags::ApplicationContextMask | EAutomationTestFlags::ProductFilter)

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
