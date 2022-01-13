#include <DLangPlugin/DLangPluginPCH.h>

#include <Core/WorldSerializer/WorldReader.h>
#include <Core/WorldSerializer/WorldWriter.h>
#include <DLangPlugin/Components/DLangComponent.h>
#include <DLangPlugin/Interop/DComponentInterfaces.h>
#include <DLangPlugin/Interop/Interop.h>

// clang-format off
EZ_BEGIN_COMPONENT_TYPE(ezDLangComponent, 1, ezComponentMode::Dynamic)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Script", GetDLangComponentFile, SetDLangComponentFile)->AddAttributes(new ezAssetBrowserAttribute("DLang")),
    EZ_MAP_ACCESSOR_PROPERTY("Parameters", GetParameters, GetParameter, SetParameter, RemoveParameter)->AddAttributes(new ezExposedParametersAttribute("Script")),
  }
  EZ_END_PROPERTIES;
  EZ_BEGIN_ATTRIBUTES
  {
    new ezCategoryAttribute("Scripting"),
  }
  EZ_END_ATTRIBUTES;
}
EZ_END_COMPONENT_TYPE;
// clang-format on

ezDLangComponent::ezDLangComponent() = default;
ezDLangComponent::~ezDLangComponent() = default;

void ezDLangComponent::SetDLangComponentGuid(const ezUuid& hResource)
{
  m_DLangComponentGuid = hResource;
}

const ezUuid& ezDLangComponent::GetDLangComponentGuid() const
{
  return m_DLangComponentGuid;
}

void ezDLangComponent::SetDLangComponentFile(const char* szFile)
{
  if (ezConversionUtils::IsStringUuid(szFile))
  {
    SetDLangComponentGuid(ezConversionUtils::ConvertStringToUuid(szFile));
  }
  else
  {
    SetDLangComponentGuid(ezUuid());
  }
}

const char* ezDLangComponent::GetDLangComponentFile() const
{
  if (m_DLangComponentGuid.IsValid())
  {
    static ezStringBuilder sGuid; // need dummy storage
    return ezConversionUtils::ToString(m_DLangComponentGuid, sGuid);
  }

  return "";
}

void ezDLangComponent::SerializeComponent(ezWorldWriter& stream) const
{
  SUPER::SerializeComponent(stream);
  auto& s = stream.GetStream();
}

void ezDLangComponent::DeserializeComponent(ezWorldReader& stream)
{
  SUPER::DeserializeComponent(stream);
  const ezUInt32 uiVersion = stream.GetComponentTypeVersion(GetStaticRTTI());
}

void ezDLangComponent::OnActivated()
{
  SUPER::OnActivated();
}

void ezDLangComponent::OnDeactivated()
{

  SUPER::OnDeactivated();
}

void ezDLangComponent::OnSimulationStarted()
{
  SUPER::OnSimulationStarted();

  m_pDLangComponent = ezDLangInterop::CreateComponentType("AsteroidsTestComponent", GetOwner(), this);

  if (m_pDLangComponent == nullptr)
    return;

  m_pDLangComponent->OnSimulationStarted();
}

void ezDLangComponent::Update()
{
  if (m_pDLangComponent == nullptr)
    return;

  m_pDLangComponent->Update();
}

const ezRangeView<const char*, ezUInt32> ezDLangComponent::GetParameters() const
{
  return ezRangeView<const char*, ezUInt32>([]() -> ezUInt32
    { return 0; },
    [this]() -> ezUInt32
    { return m_Parameters.GetCount(); },
    [](ezUInt32& it)
    { ++it; },
    [this](const ezUInt32& it) -> const char* { return m_Parameters.GetKey(it).GetString().GetData(); });
}

void ezDLangComponent::SetParameter(const char* szKey, const ezVariant& value)
{
  ezHashedString hs;
  hs.Assign(szKey);

  auto it = m_Parameters.Find(hs);
  if (it != ezInvalidIndex && m_Parameters.GetValue(it) == value)
    return;

  m_Parameters[hs] = value;

  // GetWorld()->GetComponentManager<ezDLangComponentManager>()->AddToUpdateList(this);
}

void ezDLangComponent::RemoveParameter(const char* szKey)
{
  if (m_Parameters.RemoveAndCopy(ezTempHashedString(szKey)))
  {
    // GetWorld()->GetComponentManager<ezDLangComponentManager>()->AddToUpdateList(this);
  }
}

bool ezDLangComponent::GetParameter(const char* szKey, ezVariant& out_value) const
{
  ezUInt32 it = m_Parameters.Find(szKey);

  if (it == ezInvalidIndex)
    return false;

  out_value = m_Parameters.GetValue(it);
  return true;
}
