#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/IPC/EngineProcessConnection.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OpenDdlReader.h>
#include <Foundation/Serialization/ReflectionSerializer.h>


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
    if (m_AssetProfiles[i]->GetConfigName().IsEqual_NoCase(szPlatform))
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

const ezPlatformProfile* ezAssetCurator::GetAssetProfile(ezUInt32 uiIndex) const
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[uiIndex];
}

ezPlatformProfile* ezAssetCurator::GetAssetProfile(ezUInt32 uiIndex)
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    return m_AssetProfiles[0]; // fall back to default platform

  return m_AssetProfiles[uiIndex];
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

void ezAssetCurator::SetActiveAssetProfileByIndex(ezUInt32 uiIndex, bool bForceReevaluation /*= false*/)
{
  if (uiIndex >= m_AssetProfiles.GetCount())
    uiIndex = 0; // fall back to default platform

  if (!bForceReevaluation && m_uiActiveAssetProfile == uiIndex)
    return;

  EZ_LOG_BLOCK("Switch Active Asset Platform", m_AssetProfiles[uiIndex]->GetConfigName());

  m_uiActiveAssetProfile = uiIndex;

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

void ezAssetCurator::SaveRuntimeProfiles()
{
  for (ezUInt32 i = 0; i < GetNumAssetProfiles(); ++i)
  {
    ezStringBuilder sProfileRuntimeDataFile;

    ezPlatformProfile* pProfile = GetAssetProfile(i);

    sProfileRuntimeDataFile.Set(":project/RuntimeConfigs/", pProfile->GetConfigName(), ".ezProfile");

    pProfile->SaveForRuntime(sProfileRuntimeDataFile).IgnoreResult();
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
    ddl.BeginObject("Config", pCfg->GetConfigName());

    // make sure to create the same GUID every time, otherwise the serialized file changes all the time
    const ezUuid guid = ezUuid::MakeStableUuidFromString(pCfg->GetConfigName());

    ezReflectionSerializer::WriteObjectToDDL(ddl, pCfg->GetDynamicRTTI(), pCfg, guid);

    ddl.EndObject();
  }

  ddl.EndObject();

  return file.Close();
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

      pProfile->AddMissingConfigs();

      m_AssetProfiles.PushBack(pProfile);
    }
  }

  if (m_AssetProfiles.IsEmpty() || m_AssetProfiles[0]->GetConfigName() != "Default")
  {
    ezPlatformProfile* pCfg = EZ_DEFAULT_NEW(ezPlatformProfile);
    pCfg->SetConfigName("Default");
    pCfg->SetTargetPlatform("Windows");

    pCfg->AddMissingConfigs();
    m_AssetProfiles.InsertAt(0, pCfg);
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
    pCfg->SetConfigName("Default");
    pCfg->SetTargetPlatform("Windows");
    pCfg->AddMissingConfigs();
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
