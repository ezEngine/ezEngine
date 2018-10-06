#include <PCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetProfile.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Serialization/ReflectionSerializer.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetTypeProfileConfig, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_STATIC_REFLECTED_ENUM(ezAssetProfileTargetPlatform, 1)
  EZ_ENUM_CONSTANTS(ezAssetProfileTargetPlatform::PC, ezAssetProfileTargetPlatform::Android)
EZ_END_STATIC_REFLECTED_ENUM;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetProfile, 1, ezRTTIDefaultAllocator<ezAssetProfile>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("Name", m_sName)->AddAttributes(new ezHiddenAttribute()),
    EZ_ENUM_MEMBER_PROPERTY("Platform", ezAssetProfileTargetPlatform, m_TargetPlatform),
    EZ_ARRAY_MEMBER_PROPERTY("AssetTypeConfigs", m_TypeConfigs)->AddFlags(ezPropertyFlags::PointerOwner)->AddAttributes(new ezContainerAttribute(false, false, false)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezAssetTypeProfileConfig::ezAssetTypeProfileConfig() = default;
ezAssetTypeProfileConfig::~ezAssetTypeProfileConfig() = default;

ezAssetProfile::ezAssetProfile()
{
  InitializeToDefault();
}

void func (int a = 1, int b = 2)
{
}

ezAssetProfile::~ezAssetProfile()
{
  Clear();
}

void ezAssetProfile::InitializeToDefault()
{
  Clear();

  m_TargetPlatform = ezAssetProfileTargetPlatform::Default;

  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    if (!pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract) && pRtti->IsDerivedFrom<ezAssetTypeProfileConfig>() &&
        pRtti->GetAllocator()->CanAllocate())
    {
      m_TypeConfigs.PushBack(pRtti->GetAllocator()->Allocate<ezAssetTypeProfileConfig>());
    }
  }

  m_TypeConfigs.Sort([](const ezAssetTypeProfileConfig* lhs, const ezAssetTypeProfileConfig* rhs) -> bool {
    return ezStringUtils::Compare(lhs->GetDisplayName(), rhs->GetDisplayName()) < 0;
  });
}

void ezAssetProfile::Clear()
{
  for (auto pType : m_TypeConfigs)
  {
    pType->GetDynamicRTTI()->GetAllocator()->Deallocate(pType);
  }

  m_TypeConfigs.Clear();
}


const ezAssetTypeProfileConfig* ezAssetProfile::GetTypeConfig(const ezRTTI* pRtti) const
{
  for (const auto* pConfig : m_TypeConfigs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

ezAssetTypeProfileConfig* ezAssetProfile::GetTypeConfig(const ezRTTI* pRtti)
{
  for (auto* pConfig : m_TypeConfigs)
  {
    if (pConfig->GetDynamicRTTI() == pRtti)
      return pConfig;
  }

  return nullptr;
}

//////////////////////////////////////////////////////////////////////////

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezTextureAssetTypeProfileConfig, 1, ezRTTIDefaultAllocator<ezTextureAssetTypeProfileConfig>)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("MaxResolution", m_uiMaxResolution)->AddAttributes(new ezDefaultValueAttribute(16 * 1024)),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

const char* ezTextureAssetTypeProfileConfig::GetDisplayName() const
{
  return "2D Textures";
}

//////////////////////////////////////////////////////////////////////////


const ezAssetProfile* ezAssetCurator::GetDevelopmentAssetProfile() const
{
  return m_AssetProfiles[0];
}

const ezAssetProfile* ezAssetCurator::GetActiveAssetProfile() const
{
  return m_AssetProfiles[m_uiActiveAssetProfile];
}

ezUInt32 ezAssetCurator::GetActiveAssetProfileIndex() const
{
  return m_uiActiveAssetProfile;
}

ezUInt32 ezAssetCurator::FindAssetProfileByName(const char* szPlatform)
{
  EZ_LOCK(m_CuratorMutex);

  EZ_ASSERT_DEV(!m_AssetProfiles.IsEmpty(), "Need to have a valid asset platform config");

  for (ezUInt32 i = 0; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i]->m_sName.IsEqual_NoCase(szPlatform))
    {
      return i;
    }
  }

  return ezInvalidIndex;
}

ezUInt32 ezAssetCurator::GetNumAssetProfiles() const
{
  return m_AssetProfiles.GetCount();
}

const ezAssetProfile* ezAssetCurator::GetAssetProfile(ezUInt32 index) const
{
  if (index >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[index];
}

ezAssetProfile* ezAssetCurator::GetAssetProfile(ezUInt32 index)
{
  if (index >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[index];
}

ezAssetProfile* ezAssetCurator::CreateAssetProfile()
{
  ezAssetProfile* pProfile = EZ_DEFAULT_NEW(ezAssetProfile);
  m_AssetProfiles.PushBack(pProfile);

  return pProfile;
}

ezResult ezAssetCurator::DeleteAssetProfile(ezAssetProfile* pProfile)
{
  if (m_AssetProfiles.GetCount() <= 1)
    return EZ_FAILURE;

  // do not allow to delete element 0 !

  for (ezUInt32 i = 1; i < m_AssetProfiles.GetCount(); ++i)
  {
    if (m_AssetProfiles[i] == pProfile)
    {
      if (m_uiActiveAssetProfile == i)
        return EZ_FAILURE;

      if (i < m_uiActiveAssetProfile)
        --m_uiActiveAssetProfile;

      EZ_DEFAULT_DELETE(pProfile);
      m_AssetProfiles.RemoveAtAndCopy(i);

      return EZ_SUCCESS;
    }
  }

  return EZ_FAILURE;
}

void ezAssetCurator::SetActiveAssetProfileByIndex(ezUInt32 index)
{
  if (index >= m_AssetProfiles.GetCount())
    index = 0; // fall back to default platform

  if (m_uiActiveAssetProfile == index)
    return;

  EZ_LOG_BLOCK("Switch Active Asset Platform", m_AssetProfiles[index]->GetConfigName());

  m_uiActiveAssetProfile = index;

  ezAssetCuratorEvent e;
  e.m_Type = ezAssetCuratorEvent::Type::ActivePlatformChanged;

  m_Events.Broadcast(e);

  CheckFileSystem();

  ezSimpleConfigMsgToEngine msg;
  msg.m_sWhatToDo = "ChangeActivePlatform";
  msg.m_sPayload = GetActiveAssetProfile()->GetConfigName();
  ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
}


ezResult ezAssetCurator::SaveAssetProfiles()
{
  ezDeferredFileWriter file;
  file.SetOutput(":project/Editor/AssetProfiles.ddl");

  ezOpenDdlWriter ddl;
  ddl.SetOutputStream(&file);

  ddl.BeginObject("AssetProfiles");

  for (const auto* pCfg : m_AssetProfiles)
  {
    ddl.BeginObject("Config", pCfg->m_sName);

    // make sure to create the same GUID every time, otherwise the serialized file changes all the time
    const ezUuid guid = ezUuid::StableUuidForString(pCfg->GetConfigName());

    ezReflectionSerializer::WriteObjectToDDL(ddl, pCfg->GetDynamicRTTI(), pCfg, guid);

    ddl.EndObject();
  }

  ddl.EndObject();

  return file.Close();
}

ezResult ezAssetCurator::LoadAssetProfiles()
{
  EZ_LOG_BLOCK("LoadAssetProfiles", ":project/Editor/AssetProfiles.ddl");

  ezFileReader file;
  if (file.Open(":project/Editor/AssetProfiles.ddl").Failed())
    return EZ_FAILURE;

  ezOpenDdlReader ddl;
  if (ddl.ParseDocument(file).Failed())
    return EZ_FAILURE;

  const ezOpenDdlReaderElement* pRootElement = ddl.GetRootElement()->FindChildOfType("AssetProfiles");

  if (!pRootElement)
    return EZ_FAILURE;

  if (pRootElement->FindChildOfType("Config") == nullptr)
    return EZ_FAILURE;

  ClearAssetProfiles();

  for (auto pChild = pRootElement->GetFirstChild(); pChild != nullptr; pChild = pChild->GetSibling())
  {
    if (pChild->IsCustomType("Config"))
    {
      const ezRTTI* pRtti = nullptr;
      void* pConfigObj = ezReflectionSerializer::ReadObjectFromDDL(pChild, pRtti);

      m_AssetProfiles.PushBack(static_cast<ezAssetProfile*>(pConfigObj));
    }
  }

  return EZ_SUCCESS;
}

void ezAssetCurator::ClearAssetProfiles()
{
  for (auto pCfg : m_AssetProfiles)
  {
    pCfg->GetDynamicRTTI()->GetAllocator()->Deallocate(pCfg);
  }

  m_AssetProfiles.Clear();
}

void ezAssetCurator::SetupDefaultAssetProfiles()
{
  ClearAssetProfiles();

  {
    ezAssetProfile* pCfg = EZ_DEFAULT_NEW(ezAssetProfile);
    pCfg->m_sName = "PC";
    m_AssetProfiles.PushBack(pCfg);
  }
}
