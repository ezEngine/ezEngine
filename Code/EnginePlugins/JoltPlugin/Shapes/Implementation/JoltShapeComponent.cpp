#include <JoltPlugin/JoltPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <JoltPlugin/Shapes/JoltShapeComponent.h>
#include <JoltPlugin/System/JoltCore.h>
#include <JoltPlugin/System/JoltWorldModule.h>
#include <JoltPlugin/Utilities/JoltConversionUtils.h>

// clang-format off
EZ_BEGIN_ABSTRACT_COMPONENT_TYPE(ezJoltShapeComponent, 1)
{
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Physics/Jolt/Shapes"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_ABSTRACT_COMPONENT_TYPE
// clang-format on

ezJoltShapeComponent::ezJoltShapeComponent() = default;
ezJoltShapeComponent::~ezJoltShapeComponent() = default;

void ezJoltShapeComponent::Initialize()
{
  if (IsActive())
  {
    GetOwner()->UpdateLocalBounds();
  }
}

void ezJoltShapeComponent::OnDeactivated()
{
  if (m_uiUserDataIndex != ezInvalidIndex)
  {
    ezJoltWorldModule* pModule = GetWorld()->GetModule<ezJoltWorldModule>();
    pModule->DeallocateUserData(m_uiUserDataIndex);
  }

  SUPER::OnDeactivated();
}

const ezJoltUserData* ezJoltShapeComponent::GetUserData()
{
  ezJoltWorldModule* pModule = GetWorld()->GetOrCreateModule<ezJoltWorldModule>();

  if (m_uiUserDataIndex != ezInvalidIndex)
  {
    return &pModule->GetUserData(m_uiUserDataIndex);
  }
  else
  {
    ezJoltUserData* pUserData = nullptr;
    m_uiUserDataIndex = pModule->AllocateUserData(pUserData);
    pUserData->Init(this);

    return pUserData;
  }
}

ezUInt32 ezJoltShapeComponent::GetUserDataIndex()
{
  GetUserData();
  return m_uiUserDataIndex;
}


EZ_STATICLINK_FILE(JoltPlugin, JoltPlugin_Shapes_Implementation_JoltShapeComponent);
