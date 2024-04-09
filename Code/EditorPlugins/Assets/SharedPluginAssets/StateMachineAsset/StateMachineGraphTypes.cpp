#include <SharedPluginAssets/SharedPluginAssetsPCH.h>

#include <SharedPluginAssets/StateMachineAsset/StateMachineGraphTypes.h>

#include <GameEngine/StateMachine/StateMachine.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineConnection, 1, ezRTTIDefaultAllocator<ezStateMachineConnection>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Type", m_pType)->AddFlags(ezPropertyFlags::PointerOwner)
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineNodeBase, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineNode, 1, ezRTTIDefaultAllocator<ezStateMachineNode>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezDefaultValueAttribute(ezStringView("State"))), // wrap in ezStringView to prevent a memory leak report
    EZ_MEMBER_PROPERTY("Type", m_pType)->AddFlags(ezPropertyFlags::PointerOwner),
    EZ_MEMBER_PROPERTY("IsInitialState", m_bIsInitialState)->AddAttributes(new ezHiddenAttribute()),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezStateMachineNodeAny, 1, ezRTTIDefaultAllocator<ezStateMachineNodeAny>)
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on
