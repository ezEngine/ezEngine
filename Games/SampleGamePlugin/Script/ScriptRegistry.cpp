#include <PCH.h>
#include <SampleGamePlugin/Script/ScriptRegistry.h>
#include <SampleGamePlugin/Components/ScriptTestComponent.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Logging/Log.h>

EZ_IMPLEMENT_SINGLETON(ezScriptRegistry);

EZ_BEGIN_SUBSYSTEM_DECLARATION(SampleGame, ScriptRegistry)

  BEGIN_SUBSYSTEM_DEPENDENCIES
    "Foundation"
  END_SUBSYSTEM_DEPENDENCIES

  ON_CORE_STARTUP
  {
    EZ_DEFAULT_NEW(ezScriptRegistry);
  }

  ON_CORE_SHUTDOWN
  {
    ezScriptRegistry* pDummy = ezScriptRegistry::GetSingleton();
    EZ_DEFAULT_DELETE(pDummy);
  }

  ON_ENGINE_STARTUP
  {
  }

  ON_ENGINE_SHUTDOWN
  {
  }

EZ_END_SUBSYSTEM_DECLARATION

//////////////////////////////////////////////////////////////////////////


ezScriptMemberProperty::ezScriptMemberProperty(const char* szName, const ezRTTI* pType)
  : ezAbstractMemberProperty(nullptr)
{
  m_sPropertyNameStorage = szName;
  m_szPropertyName = m_sPropertyNameStorage.GetData();
  m_pPropertyType = pType;

  ezVariant::Type::Enum type = pType->GetVariantType();
  if ((type >= ezVariant::Type::FirstStandardType && type <= ezVariant::Type::LastStandardType) || pType == ezGetStaticRTTI<ezVariant>())
    m_Flags.Add(ezPropertyFlags::StandardType);

  if (type == ezVariant::Type::VoidPointer || type == ezVariant::Type::ReflectedPointer)
    m_Flags.Add(ezPropertyFlags::Pointer);

  if (!m_Flags.IsAnySet(ezPropertyFlags::StandardType | ezPropertyFlags::Pointer))
    m_Flags.Add(ezPropertyFlags::Class);
}

ezScriptMemberProperty::~ezScriptMemberProperty()
{

}

const ezRTTI* ezScriptMemberProperty::GetSpecificType() const
{
  return m_pPropertyType;
}

void ezScriptMemberProperty::GetValuePtr(const void* pInstance, void* pObject) const
{
  const ScriptTestComponent* pComp = static_cast<const ScriptContainerBase*>(pInstance)->m_pComponent;
  pComp->GetProperty(this, pObject);
}

void ezScriptMemberProperty::SetValuePtr(void* pInstance, void* pObject)
{
  ScriptTestComponent* pComp = static_cast<ScriptContainerBase*>(pInstance)->m_pComponent;
  pComp->SetProperty(this, pObject);
}

//////////////////////////////////////////////////////////////////////////

ezScriptRTTI::~ezScriptRTTI()
{
  UnregisterType(this);
  m_szTypeName = nullptr;

  for (auto pProp : m_PropertiesStorage)
  {
    EZ_DEFAULT_DELETE(pProp);
  }
  for (auto pAttrib : m_AttributesStorage)
  {
    EZ_DEFAULT_DELETE(pAttrib);
  }
  EZ_DEFAULT_DELETE(m_pAllocator);
}

ezScriptRTTI::ezScriptRTTI(const char* szName, const ezRTTI* pParentType, ezUInt32 uiTypeSize, ezUInt32 uiTypeVersion, ezUInt32 uiVariantType, ezBitflags<ezTypeFlags> flags, const char* szPluginName)
  : ezRTTI(nullptr, pParentType, uiTypeSize, uiTypeVersion, uiVariantType, flags, nullptr, ezArrayPtr<ezAbstractProperty*>(), ezArrayPtr<ezPropertyAttribute*>(), ezArrayPtr<ezAbstractMessageHandler*>(), ezArrayPtr<ezMessageSenderInfo>(), nullptr)
{
  m_sTypeNameStorage = szName;
  m_sPluginNameStorage = szPluginName;

  m_szTypeName = m_sTypeNameStorage.GetData();
  m_szPluginName = m_sPluginNameStorage.GetData();

  m_pAllocator = EZ_DEFAULT_NEW(ezScriptContainerAllocator, this);
  RegisterType(this);
}

void ezScriptRTTI::SetProperties(ezArrayPtr<ezAbstractProperty*> properties)
{
  for (auto pProp : m_PropertiesStorage)
  {
    EZ_DEFAULT_DELETE(pProp);
  }
  m_PropertiesStorage.Clear();
  m_PropertiesStorage = properties;
  m_Properties = m_PropertiesStorage;
}

void ezScriptRTTI::SetAttributes(ezArrayPtr<ezPropertyAttribute*> attributes)
{
  for (auto pAttrib : m_AttributesStorage)
  {
    EZ_DEFAULT_DELETE(pAttrib);
  }
  m_AttributesStorage.Clear();
  m_AttributesStorage = attributes;
  m_Attributes = m_AttributesStorage;
}

//////////////////////////////////////////////////////////////////////////

ezScriptRegistry::ezScriptRegistry()
  : m_SingletonRegistrar(this)
{
}

void ezScriptRegistry::UpdateScriptTypes(const char* szScriptPath)
{
  if (ezStringUtils::IsNullOrEmpty(szScriptPath))
    return;

  ezStringBuilder sShaderPath = szScriptPath;
  sShaderPath.MakeCleanPath();

  auto it = m_ShaderTypes.Find(sShaderPath);
  if (it.IsValid())
  {
    ezStringBuilder sAbsPath = sShaderPath;
    if (!ezFileSystem::ResolvePath(sShaderPath, &sAbsPath, nullptr).Succeeded())
    {
      ezLog::Error("Can't make script path absolute: '{0}'", sShaderPath);
      return;
    }
    ezFileStats Stats;
    if (ezOSFile::GetFileStats(sAbsPath, Stats).Succeeded() &&
      !Stats.m_LastModificationTime.Compare(it.Value().m_fileModifiedTime, ezTimestamp::CompareMode::FileTimeEqual))
    {
      UpdateScriptTypes(it.Value());
    }
  }
  else
  {
    it = m_ShaderTypes.Insert(sShaderPath, ScriptData());
    it.Value().m_sScriptPath = sShaderPath;
    UpdateScriptTypes(it.Value());
  }
}

void ezScriptRegistry::UpdateScriptTypes(ScriptData& data)
{
  EZ_LOG_BLOCK("Updating Script Parameters", data.m_sScriptPath.GetData());

  {
    ezStringBuilder sAbsPath;
    if (!ezFileSystem::ResolvePath(data.m_sScriptPath, &sAbsPath, nullptr).Succeeded())
    {
      ezLog::Error("Can't make script path absolute: '{0}'", data.m_sScriptPath);
      return;
    }
    ezFileStats Stats;
    bool bStat = ezOSFile::GetFileStats(sAbsPath, Stats).Succeeded();

    ezFileReader file;
    if (!bStat || file.Open(sAbsPath).Failed())
    {
      ezLog::Error("Can't update script '{0}' type information, the file can't be opened.", data.m_sScriptPath.GetData());
      return;
    }

    data.m_fileModifiedTime = Stats.m_LastModificationTime;
  }

  // Pull an imaginary type and properties out of my ass
  ezStringBuilder sTypeName = ezPathUtils::GetFileName(data.m_sScriptPath);
  sTypeName.ReplaceAll(" ", "_");
  sTypeName.Append("_TestFunction");
  ezScriptRTTI* pPhantom = nullptr;
  {
    ezRTTI* pType = ezRTTI::FindTypeByName(sTypeName);
    EZ_ASSERT_DEBUG(!pType || data.m_pTypes.Contains(pType), "Type exists already but does not belong to our script!");
    pPhantom = pType ? static_cast<ezScriptRTTI*>(pType) : nullptr;
  }

  if (pPhantom == nullptr)
  {
    pPhantom = EZ_DEFAULT_NEW(ezScriptRTTI, sTypeName, ezGetStaticRTTI<ScriptContainerBase>(),
      sizeof(ScriptContainerBase), 0, ezVariantType::Invalid, ezTypeFlags::Class, "SampleGamePlugin");
    data.m_pTypes.PushBack(pPhantom);
  }

  ezHybridArray<ezAbstractProperty*, 4> properties;
  properties.PushBack(EZ_DEFAULT_NEW(ezConstantProperty<ezString>, "ScriptSource", data.m_sScriptPath));
  properties.PushBack(EZ_DEFAULT_NEW(ezScriptMemberProperty, "TestString", ezGetStaticRTTI<ezString>()));
  properties.PushBack(EZ_DEFAULT_NEW(ezScriptMemberProperty, "TestInt", ezGetStaticRTTI<ezInt32>()));
  pPhantom->SetProperties(properties);

  ezHybridArray<ezPropertyAttribute*, 4> attributes;
  pPhantom->SetAttributes(attributes);

  // Inform e.g. editor of new type.
  ezRTTI::s_TypeUpdatedEvent.Broadcast(pPhantom);
}
