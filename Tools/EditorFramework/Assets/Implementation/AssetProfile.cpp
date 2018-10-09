#include <PCH.h>

#include <EditorEngineProcessFramework/EngineProcess/EngineProcessMessages.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/IO/OpenDdlWriter.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GameEngine/Configuration/PlatformProfile.h>


const ezPlatformProfile* ezAssetCurator::GetDevelopmentAssetProfile() const
{
  return m_AssetProfiles[0];
}

const ezPlatformProfile* ezAssetCurator::GetActiveAssetProfile() const
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

const ezPlatformProfile* ezAssetCurator::GetAssetProfile(ezUInt32 index) const
{
  if (index >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[index];
}

ezPlatformProfile* ezAssetCurator::GetAssetProfile(ezUInt32 index)
{
  if (index >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[index];
}

ezPlatformProfile* ezAssetCurator::CreateAssetProfile()
{
  ezPlatformProfile* pProfile = EZ_DEFAULT_NEW(ezPlatformProfile);
  m_AssetProfiles.PushBack(pProfile);

  return pProfile;
}

ezResult ezAssetCurator::DeleteAssetProfile(ezPlatformProfile* pProfile)
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

void ezAssetCurator::SetActiveAssetProfileByIndex(ezUInt32 index, bool bForceReevaluation /*= false*/)
{
  if (index >= m_AssetProfiles.GetCount())
    index = 0; // fall back to default platform

  if (!bForceReevaluation && m_uiActiveAssetProfile == index)
    return;

  EZ_LOG_BLOCK("Switch Active Asset Platform", m_AssetProfiles[index]->GetConfigName());

  m_uiActiveAssetProfile = index;

  CheckFileSystem();

  {
    ezAssetCuratorEvent e;
    e.m_Type = ezAssetCuratorEvent::Type::ActivePlatformChanged;
    m_Events.Broadcast(e);
  }

  {
    ezSimpleConfigMsgToEngine msg;
    msg.m_sWhatToDo = "ChangeActivePlatform";
    msg.m_sPayload = GetActiveAssetProfile()->GetConfigName();
    ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&msg);
  }
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

void AddMissingConfigs(ezPlatformProfile* pProfile)
{
  for (auto pRtti = ezRTTI::GetFirstInstance(); pRtti != nullptr; pRtti = pRtti->GetNextInstance())
  {
    // find all types derived from ezProfileConfigData
    if (!pRtti->GetTypeFlags().IsAnySet(ezTypeFlags::Abstract) && pRtti->IsDerivedFrom<ezProfileConfigData>() &&
        pRtti->GetAllocator()->CanAllocate())
    {
      bool bHasTypeAlready = false;

      // check whether we already have an instance of this type
      for (auto pType : pProfile->m_Configs)
      {
        if (pType->GetDynamicRTTI() == pRtti)
        {
          bHasTypeAlready = true;
          break;
        }
      }

      if (!bHasTypeAlready)
      {
        // if not, allocate one
        ezProfileConfigData* pObject = pRtti->GetAllocator()->Allocate<ezProfileConfigData>();
        ezToolsReflectionUtils::SetAllMemberPropertiesToDefault(pRtti, pObject);

        pProfile->m_Configs.PushBack(pObject);
      }
    }
  }

  // sort all configs alphabetically
  pProfile->m_Configs.Sort([](const ezProfileConfigData* lhs, const ezProfileConfigData* rhs) -> bool {
    return ezStringUtils::Compare(lhs->GetDynamicRTTI()->GetTypeName(), rhs->GetDynamicRTTI()->GetTypeName()) < 0;
  });
}

ezResult ezAssetCurator::LoadAssetProfiles()
{
  EZ_LOG_BLOCK("LoadAssetProfiles", ":project/Editor/PlatformProfiles.ddl");

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

      auto pProfile = static_cast<ezPlatformProfile*>(pConfigObj);

      AddMissingConfigs(pProfile);

      m_AssetProfiles.PushBack(pProfile);
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
    ezPlatformProfile* pCfg = EZ_DEFAULT_NEW(ezPlatformProfile);
    pCfg->m_sName = "PC";
    AddMissingConfigs(pCfg);
    m_AssetProfiles.PushBack(pCfg);
  }
}

void ezAssetCurator::ComputeAllDocumentManagerAssetProfileHashes()
{
  for (auto pMan : ezDocumentManager::GetAllDocumentManagers())
  {
    if (auto pAssMan = ezDynamicCast<ezAssetDocumentManager*>(pMan))
    {
      pAssMan->ComputeAssetProfileHash(GetActiveAssetProfile());
    }
  }
}
