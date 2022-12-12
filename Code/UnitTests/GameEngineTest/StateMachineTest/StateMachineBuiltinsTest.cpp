#include <GameEngineTest/GameEngineTestPCH.h>

#include "StateMachineTest.h"
#include <GameEngine/StateMachine/StateMachineBuiltins.h>

namespace
{
  class TestState : public ezStateMachineState
  {
    EZ_ADD_DYNAMIC_REFLECTION(TestState, ezStateMachineState);

  public:
    TestState(ezStringView sName = ezStringView())
      : ezStateMachineState(sName)
    {
    }

    virtual void OnEnter(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pFromState) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_Counter.m_uiEnterCounter++;

      m_CounterTable[&instance] = pData->m_Counter;
    }

    virtual void OnExit(ezStateMachineInstance& instance, void* pInstanceData, const ezStateMachineState* pToState) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_Counter.m_uiExitCounter++;

      m_CounterTable[&instance] = pData->m_Counter;
    }

    virtual bool GetInstanceDataDesc(ezStateMachineInstanceDataDesc& out_desc) override
    {
      out_desc.FillFromType<InstanceData>();
      return true;
    }

    struct Counter
    {
      ezUInt32 m_uiEnterCounter = 0;
      ezUInt32 m_uiExitCounter = 0;
    };

    mutable ezHashTable<ezStateMachineInstance*, Counter> m_CounterTable;

    struct InstanceData
    {
      InstanceData() { s_uiConstructionCounter++; }
      ~InstanceData() { s_uiDestructionCounter++; }

      Counter m_Counter;

      static ezUInt32 s_uiConstructionCounter;
      static ezUInt32 s_uiDestructionCounter;
    };
  };

  ezUInt32 TestState::InstanceData::s_uiConstructionCounter = 0;
  ezUInt32 TestState::InstanceData::s_uiDestructionCounter = 0;

  // clang-format off
  EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(TestState, 1, ezRTTIDefaultAllocator<TestState>)
  EZ_END_DYNAMIC_REFLECTED_TYPE;
  // clang-format on

  class TestTransition : public ezStateMachineTransition
  {
  public:
    bool IsConditionMet(ezStateMachineInstance& instance, void* pInstanceData) const override
    {
      auto pData = static_cast<InstanceData*>(pInstanceData);
      pData->m_uiConditionCounter++;

      return pData->m_uiConditionCounter > 1;
    }

    bool GetInstanceDataDesc(ezStateMachineInstanceDataDesc& out_desc) override
    {
      out_desc.FillFromType<InstanceData>();
      return true;
    }

    struct InstanceData
    {
      InstanceData() { s_uiConstructionCounter++; }
      ~InstanceData() { s_uiDestructionCounter++; }

      ezUInt32 m_uiConditionCounter;

      static ezUInt32 s_uiConstructionCounter;
      static ezUInt32 s_uiDestructionCounter;
    };
  };

  ezUInt32 TestTransition::InstanceData::s_uiConstructionCounter = 0;
  ezUInt32 TestTransition::InstanceData::s_uiDestructionCounter = 0;

  static void ResetCounter()
  {
    TestState::InstanceData::s_uiConstructionCounter = 0;
    TestState::InstanceData::s_uiDestructionCounter = 0;
    TestTransition::InstanceData::s_uiConstructionCounter = 0;
    TestTransition::InstanceData::s_uiDestructionCounter = 0;
  }

  static ezTime s_TimeStep = ezTime::Milliseconds(10);

} // namespace

void ezGameEngineTestStateMachine::RunBuiltinsTest()
{
  ezReflectedClass fakeOwner;

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Simple States")
  {
    ResetCounter();

    ezSharedPtr<ezStateMachineDescription> pDesc = EZ_DEFAULT_NEW(ezStateMachineDescription);

    auto pStateA = EZ_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = EZ_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pTransition = EZ_DEFAULT_NEW(TestTransition);
    pDesc->AddTransition(1, 0, pTransition);

    ezStateMachineInstance* pInstance = nullptr;
    {
      ezStateMachineInstance sm(fakeOwner, pDesc);
      EZ_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 2);
      EZ_TEST_INT(TestTransition::InstanceData::s_uiConstructionCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      ezHashedString sStateName; // intentionally left empty to go to fallback state (state with index 0 -> state "A")
      EZ_TEST_BOOL(sm.SetStateOrFallback(sStateName).Succeeded());
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      EZ_TEST_BOOL(sm.SetState(pStateB).Succeeded());
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      sStateName.Assign("C");
      EZ_TEST_BOOL(sm.SetState(sStateName).Failed());

      // no transition yet
      sm.Update(s_TimeStep);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // go back to "A"
      sm.Update(s_TimeStep);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 2);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 1);

      pInstance = &sm; // will be dead after this line but we only need the pointer
    }

    EZ_TEST_INT(TestState::InstanceData::s_uiDestructionCounter, 2);
    EZ_TEST_INT(TestTransition::InstanceData::s_uiDestructionCounter, 1);
    EZ_TEST_INT(pStateA->m_CounterTable[pInstance].m_uiEnterCounter, 2);
    EZ_TEST_INT(pStateA->m_CounterTable[pInstance].m_uiExitCounter, 2);
    EZ_TEST_INT(pStateB->m_CounterTable[pInstance].m_uiEnterCounter, 1);
    EZ_TEST_INT(pStateB->m_CounterTable[pInstance].m_uiExitCounter, 1);
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Blackboard Transition")
  {
    ResetCounter();

    ezSharedPtr<ezStateMachineDescription> pDesc = EZ_DEFAULT_NEW(ezStateMachineDescription);

    auto pStateA = EZ_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = EZ_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pStateC = EZ_DEFAULT_NEW(TestState, "C");
    pDesc->AddState(pStateC);

    ezHashedString sTestVal = ezMakeHashedString("TestVal");
    ezHashedString sTestVal2 = ezMakeHashedString("TestVal2");

    {
      auto pTransition = EZ_DEFAULT_NEW(ezStateMachineTransition_BlackboardConditions);
      auto& cond = pTransition->m_Conditions.ExpandAndGetRef();
      cond.m_sEntryName = sTestVal;
      cond.m_fComparisonValue = 2;
      cond.m_Operator = ezComparisonOperator::Greater;

      auto& cond2 = pTransition->m_Conditions.ExpandAndGetRef();
      cond2.m_sEntryName = sTestVal2;
      cond2.m_fComparisonValue = 10;
      cond2.m_Operator = ezComparisonOperator::Equal;

      pDesc->AddTransition(0, 1, pTransition);
    }

    {
      auto pTransition = EZ_DEFAULT_NEW(ezStateMachineTransition_BlackboardConditions);
      pTransition->m_Operator = ezStateMachineLogicOperator::Or;

      auto& cond = pTransition->m_Conditions.ExpandAndGetRef();
      cond.m_sEntryName = sTestVal;
      cond.m_fComparisonValue = 3;
      cond.m_Operator = ezComparisonOperator::Greater;

      auto& cond2 = pTransition->m_Conditions.ExpandAndGetRef();
      cond2.m_sEntryName = sTestVal2;
      cond2.m_fComparisonValue = 20;
      cond2.m_Operator = ezComparisonOperator::Equal;

      pDesc->AddTransition(1, 2, pTransition);
    }

    {
      ezSharedPtr<ezBlackboard> pBlackboard = ezBlackboard::Create();
      pBlackboard->RegisterEntry(sTestVal, 2);
      pBlackboard->RegisterEntry(sTestVal2, 0);

      ezStateMachineInstance sm(fakeOwner, pDesc);
      sm.SetBlackboard(pBlackboard);
      EZ_TEST_BOOL(sm.SetState(pStateA).Succeeded());

      // no transition yet since only part of the conditions is true
      EZ_TEST_BOOL(pBlackboard->SetEntryValue(sTestVal, 3).Succeeded());
      sm.Update(s_TimeStep);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
      EZ_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);

      // transition to B
      EZ_TEST_BOOL(pBlackboard->SetEntryValue(sTestVal2, 10).Succeeded());
      sm.Update(s_TimeStep);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
      EZ_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);

      // transition to C, only part of the condition needed because of 'OR' operator
      EZ_TEST_BOOL(pBlackboard->SetEntryValue(sTestVal2, 20).Succeeded());
      sm.Update(s_TimeStep);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(pStateC->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateC->m_CounterTable[&sm].m_uiExitCounter, 0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Timeout Transition")
  {
    ResetCounter();

    ezSharedPtr<ezStateMachineDescription> pDesc = EZ_DEFAULT_NEW(ezStateMachineDescription);

    auto pStateA = EZ_DEFAULT_NEW(TestState, "A");
    pDesc->AddState(pStateA);

    auto pStateB = EZ_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    auto pTransition = EZ_DEFAULT_NEW(ezStateMachineTransition_Timeout);
    pTransition->m_Timeout = ezTime::Milliseconds(5);
    pDesc->AddTransition(0, 1, pTransition);

    {
      ezStateMachineInstance sm(fakeOwner, pDesc);
      EZ_TEST_BOOL(sm.SetState(pStateA).Succeeded());

      sm.Update(s_TimeStep);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      sm.Update(s_TimeStep);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateA->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
    }
  }

  EZ_TEST_BLOCK(ezTestBlock::Enabled, "Compounds")
  {
    ResetCounter();

    ezSharedPtr<ezStateMachineDescription> pDesc = EZ_DEFAULT_NEW(ezStateMachineDescription);

    auto pCompoundState = EZ_DEFAULT_NEW(ezStateMachineState_Compound, "A");
    {
      auto pAllocator = ezGetStaticRTTI<TestState>()->GetAllocator();
      pCompoundState->m_SubStates.PushBack(pAllocator->Allocate<TestState>());
      pCompoundState->m_SubStates.PushBack(pAllocator->Allocate<TestState>());
    }
    pDesc->AddState(pCompoundState);

    auto pStateB = EZ_DEFAULT_NEW(TestState, "B");
    pDesc->AddState(pStateB);

    ezHashedString sTestVal = ezMakeHashedString("TestVal");

    {
      auto pCompoundTransition = EZ_DEFAULT_NEW(ezStateMachineTransition_Compound);

      {
        auto pAllocator = ezGetStaticRTTI<ezStateMachineTransition_BlackboardConditions>()->GetAllocator();
        auto pSubTransition = pAllocator->Allocate<ezStateMachineTransition_BlackboardConditions>();

        auto& cond = pSubTransition->m_Conditions.ExpandAndGetRef();
        cond.m_sEntryName = sTestVal;
        cond.m_fComparisonValue = 2;
        cond.m_Operator = ezComparisonOperator::Greater;

        pCompoundTransition->m_SubTransitions.PushBack(pSubTransition);
      }

      {
        auto pAllocator = ezGetStaticRTTI<ezStateMachineTransition_Timeout>()->GetAllocator();
        auto pSubTransition = pAllocator->Allocate<ezStateMachineTransition_Timeout>();
        pSubTransition->m_Timeout = ezTime::Milliseconds(5);

        pCompoundTransition->m_SubTransitions.PushBack(pSubTransition);
      }

      pDesc->AddTransition(0, 1, pCompoundTransition);
    }

    {
      ezSharedPtr<ezBlackboard> pBlackboard = ezBlackboard::Create();
      pBlackboard->RegisterEntry(sTestVal, 2);

      ezStateMachineInstance sm(fakeOwner, pDesc);
      sm.SetBlackboard(pBlackboard);
      EZ_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 1); // Compound instance data not constructed yet

      EZ_TEST_BOOL(sm.SetState(pCompoundState).Succeeded());
      EZ_TEST_INT(TestState::InstanceData::s_uiConstructionCounter, 3);
      EZ_TEST_INT(ezStaticCast<TestState*>(pCompoundState->m_SubStates[0])->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(ezStaticCast<TestState*>(pCompoundState->m_SubStates[1])->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // no transition yet because timeout is not reached yet
      EZ_TEST_BOOL(pBlackboard->SetEntryValue(sTestVal, 3).Succeeded());
      sm.Update(s_TimeStep);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 0);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);

      // all conditions met, transition to B
      sm.Update(s_TimeStep);
      EZ_TEST_INT(ezStaticCast<TestState*>(pCompoundState->m_SubStates[0])->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(ezStaticCast<TestState*>(pCompoundState->m_SubStates[1])->m_CounterTable[&sm].m_uiExitCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiEnterCounter, 1);
      EZ_TEST_INT(pStateB->m_CounterTable[&sm].m_uiExitCounter, 0);
    }

    EZ_TEST_INT(TestState::InstanceData::s_uiDestructionCounter, 3);
  }
}
