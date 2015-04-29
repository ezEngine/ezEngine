#include <PCH.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentInfo, ezDocumentInfo, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
EZ_ACCESSOR_PROPERTY("Dependencies", GetDependencies, SetDependencies),
EZ_ACCESSOR_PROPERTY("References", GetReferences, SetReferences),
EZ_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
EZ_END_PROPERTIES
EZ_END_DYNAMIC_REFLECTED_TYPE();

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocument, ezDocumentBase, 1, ezRTTINoAllocator);
EZ_END_DYNAMIC_REFLECTED_TYPE();

ezAssetDocumentInfo::ezAssetDocumentInfo()
{
  m_uiSettingsHash = 0;
}

ezString ezAssetDocumentInfo::GetDependencies() const
{
  ezStringBuilder s;

  for (const auto& dep : m_FileDependencies)
  {
    s.AppendFormat("%s;", dep.GetData());
  }

  s.Shrink(0, 1);
  return s;
}

void ezAssetDocumentInfo::SetDependencies(ezString s)
{
  m_FileDependencies.Clear();

  ezStringBuilder sTemp = s;
  sTemp.MakeCleanPath();
  sTemp.Split(false, m_FileDependencies, ";");
}


ezString ezAssetDocumentInfo::GetReferences() const
{
  ezStringBuilder s;

  for (const auto& dep : m_FileReferences)
  {
    s.AppendFormat("%s;", dep.GetData());
  }

  s.Shrink(0, 1);
  return s;
}

void ezAssetDocumentInfo::SetReferences(ezString s)
{
  m_FileReferences.Clear();

  ezStringBuilder sTemp = s;
  sTemp.Split(false, m_FileReferences, ";");
}


ezAssetDocument::ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManagerBase* pObjectManager) : ezDocumentBase(szDocumentPath, pObjectManager)
{
}

ezAssetDocument::~ezAssetDocument()
{
}

ezDocumentInfo* ezAssetDocument::CreateDocumentInfo()
{
  return EZ_DEFAULT_NEW(ezAssetDocumentInfo);
}

ezStatus ezAssetDocument::InternalSaveDocument()
{
  UpdateAssetDocumentInfo(static_cast<ezAssetDocumentInfo*>(m_pDocumentInfo));
  return ezDocumentBase::InternalSaveDocument();
}

ezUInt64 ezAssetDocument::GetDocumentHash() const
{
  ezUInt64 uiHash = 0;
  for (auto pChild : GetObjectTree()->GetRootObject()->GetChildren())
  {
    GetChildHash(pChild, uiHash);
  }

  return uiHash;
}

void ezAssetDocument::GetChildHash(const ezDocumentObjectBase* pObject, ezUInt64& uiHash) const
{
  pObject->ComputeObjectHash(uiHash);

  for (auto pChild : pObject->GetChildren())
  {
    GetChildHash(pChild, uiHash);
  }
}

ezStatus ezAssetDocument::TransformAsset(const char* szPlatform)
{
  ezString sPlatform = szPlatform;

  if (sPlatform.IsEmpty())
    sPlatform = ezAssetCurator::GetInstance()->GetActivePlatform();

  ezUInt64 uiHash = ezAssetCurator::GetInstance()->GetAssetDependencyHash(GetGuid());

  EZ_ASSERT_DEV(uiHash != 0, "Something went wrong");

  const ezString sResourceFile = static_cast<ezAssetDocumentManager*>(GetDocumentManager())->GenerateResourceFileName(GetDocumentPath(), sPlatform);

  if (ezAssetDocumentManager::IsResourceUpToDate(uiHash, sResourceFile))
    return ezStatus(EZ_SUCCESS);

  // Write resource
  ezFileWriter file;

  if (file.Open(sResourceFile).Failed())
  {
    ezLog::Error("Could not open file for writing: '%s'", sResourceFile.GetData());
    return ezStatus("Opening the asset output file failed");
  }

  file << uiHash;

  return InternalTransformAsset(file, sPlatform);
}

