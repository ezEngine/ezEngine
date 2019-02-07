#include <EditorFrameworkPCH.h>

#include <EditorFramework/GUI/ExposedParameters.h>

// clang-format off
EZ_BEGIN_STATIC_REFLECTED_TYPE(ezExposedParameter, ezNoBase, 2, ezRTTIDefaultAllocator<ezExposedParameter>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName),
    EZ_MEMBER_PROPERTY("Type", m_sType),
    EZ_MEMBER_PROPERTY("DefaultValue", m_DefaultValue),
    EZ_ARRAY_MEMBER_PROPERTY("Attributes", m_Attributes)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_STATIC_REFLECTED_TYPE;

ezExposedParameter::ezExposedParameter()
{
}

ezExposedParameter::~ezExposedParameter()
{
  for (auto pAttr : m_Attributes)
  {
    pAttr->GetDynamicRTTI()->GetAllocator()->Deallocate(pAttr);
  }
}

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezExposedParameters, 3, ezRTTIDefaultAllocator<ezExposedParameters>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ARRAY_MEMBER_PROPERTY("Parameters", m_Parameters)->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezExposedParameters::ezExposedParameters() {}

ezExposedParameters::~ezExposedParameters()
{
  for (auto pAttr : m_Parameters)
  {
    ezGetStaticRTTI<ezExposedParameter>()->GetAllocator()->Deallocate(pAttr);
  }
}

const ezExposedParameter* ezExposedParameters::Find(const char* szParamName) const
{
  const ezExposedParameter*const* pParam = std::find_if(cbegin(m_Parameters), cend(m_Parameters),
    [szParamName](const ezExposedParameter* param) { return param->m_sName == szParamName; });
  return pParam != cend(m_Parameters) ? *pParam : nullptr;
}
