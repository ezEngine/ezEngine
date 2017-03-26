#include <PCH.h>
#include <SampleGamePlugin/Components/ScriptTestComponent.h>
#include <SampleGamePlugin/Script/ScriptRegistry.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <Core/WorldSerializer/WorldReader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ScriptContainerBase, 1, ezRTTIDefaultAllocator<ScriptContainerBase>)
{
}
EZ_END_DYNAMIC_REFLECTED_TYPE

ScriptContainerBase::ScriptContainerBase()
{

}

EZ_BEGIN_COMPONENT_TYPE(ScriptTestComponent, 2)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("ScriptPath", GetScript, SetScript)->AddAttributes(new ezFileBrowserAttribute("Browse Script", "*.txt")),
    EZ_ACCESSOR_PROPERTY("ScriptContainer", GetScriptContainer, SetScriptContainer)->AddAttributes(new ezConstrainPointerAttribute("ScriptSource", "ScriptPath"))->AddFlags(ezPropertyFlags::PointerOwner),
  }
  EZ_END_PROPERTIES

  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("SampleGame"),
  }
  EZ_END_ATTRIBUTES
}
EZ_END_COMPONENT_TYPE

namespace
{
  struct MakeVariantFunc
  {
    template <typename T>
    EZ_FORCE_INLINE void operator()()
    {
      T* value = (T*)pValue;
      m_Result = *value;
    }

    void* pValue;
    ezVariant m_Result;
  };
}

ScriptTestComponent::ScriptTestComponent()
{
}


ScriptTestComponent::~ScriptTestComponent()
{
  if (m_pScriptContainer)
  {
    m_pScriptContainer->GetDynamicRTTI()->GetAllocator()->Deallocate(m_pScriptContainer);
    m_pScriptContainer = nullptr;
  }
}

void ScriptTestComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);

  auto& s = stream.GetStream();

  // Version 1
  s << m_sScriptPath;
  // TODO m_pScriptContainer
}


void ScriptTestComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());

  auto& s = stream.GetStream();

  s >> m_sScriptPath;
  // TODO m_pScriptContainer
}

void ScriptTestComponent::SetScript(const char* szScriptPath)
{
  m_sScriptPath = szScriptPath;
  ezScriptRegistry::GetSingleton()->UpdateScriptTypes(szScriptPath);
}

const char* ScriptTestComponent::GetScript() const
{
  return m_sScriptPath;
}

void ScriptTestComponent::SetScriptContainer(ScriptContainerBase* pContainer)
{
  if (m_pScriptContainer)
  {
    m_pScriptContainer->m_pComponent = nullptr;
  }
  m_pScriptContainer = pContainer;
  if (m_pScriptContainer)
  {
    m_pScriptContainer->m_pComponent = this;
  }
}

ScriptContainerBase* ScriptTestComponent::GetScriptContainer() const
{
  return m_pScriptContainer;
}

void ScriptTestComponent::SetProperty(const ezScriptMemberProperty* pProp, void* pValue)
{
  ezVariantType::Enum type = pProp->GetSpecificType()->GetVariantType();
  if (type != ezVariantType::Invalid)
  {
    MakeVariantFunc func;
    func.pValue = pValue;
    ezVariant::DispatchTo(func, type);
    ezLog::Info("Set property '{0}' to value '{1}'", pProp->GetPropertyName(), func.m_Result.ConvertTo<ezString>());
  }
}

void ScriptTestComponent::GetProperty(const ezScriptMemberProperty* pProp, void* pValue) const
{

}

void ScriptTestComponent::OnSimulationStarted()
{
  ScriptTestComponentManager* pManager = GetWorld()->GetOrCreateComponentManager<ScriptTestComponentManager>();
}

void ScriptTestComponent::Update()
{
  // do stuff
}

void ScriptTestComponent::Initialize()
{

}

