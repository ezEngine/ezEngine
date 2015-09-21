#include <PCH.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <ToolsFoundation/Project/ToolsProject.h>
#include <ToolsFoundation/Object/DocumentObjectManager.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/Logging/Log.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <CoreUtils/Image/Image.h>
#include <CoreUtils/Image/ImageConversion.h>
#include <QImage>
#include <QPainter>
#include <Foundation/IO/OSFile.h>
#include <CoreUtils/Assets/AssetFileHeader.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentInfo, ezDocumentInfo, 1, ezRTTINoAllocator);
EZ_BEGIN_PROPERTIES
EZ_ACCESSOR_PROPERTY("Dependencies", GetDependencies, SetDependencies),
EZ_ACCESSOR_PROPERTY("References", GetReferences, SetReferences),
EZ_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
EZ_MEMBER_PROPERTY("AssetType", m_sAssetTypeName),
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
    if (!dep.IsEmpty())
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


ezAssetDocument::ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager) : ezDocumentBase(szDocumentPath, pObjectManager)
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
  ezAssetDocumentInfo* pInfo = static_cast<ezAssetDocumentInfo*>(m_pDocumentInfo);

  pInfo->m_FileDependencies.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetTypeName = QueryAssetType();

  UpdateAssetDocumentInfo(pInfo);

  return ezDocumentBase::InternalSaveDocument();
}

ezUInt64 ezAssetDocument::GetDocumentHash() const
{
  ezUInt64 uiHash = 0;
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
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
  if (IsModified())
  {
    auto res = SaveDocument().m_Result;
    if (res.Failed())
      return res;
  }

  ezString sPlatform = szPlatform;

  if (sPlatform.IsEmpty())
    sPlatform = ezAssetCurator::GetInstance()->GetActivePlatform();

  const ezUInt64 uiHash = ezAssetCurator::GetInstance()->GetAssetDependencyHash(GetGuid());

  if (uiHash == 0)
    return ezStatus("Computing the hash for this asset or any dependency failed");

  const ezString sResourceFile = static_cast<ezAssetDocumentManager*>(GetDocumentManager())->GenerateResourceFileName(GetDocumentPath(), sPlatform);

  if (ezAssetDocumentManager::IsResourceUpToDate(uiHash, sResourceFile))
    return ezStatus(EZ_SUCCESS);

  // Write resource
  {
    ezFileWriter file;

    if (file.Open(sResourceFile).Failed())
    {
      ezLog::Error("Could not open file for writing: '%s'", sResourceFile.GetData());
      return ezStatus("Opening the asset output file failed");
    }

    ezAssetFileHeader AssetHeader;
    AssetHeader.SetFileHash(uiHash);

    AssetHeader.Write(file);

    auto ret = InternalTransformAsset(file, sPlatform);

    // if writing failed, make sure the output file does not exist
    if (ret.m_Result.Failed())
    {
      file.Close();
      ezFileSystem::DeleteFile(sResourceFile);

      /// \todo Write to dummy file, then rename it
    }

    return ret;
  }
}

ezStatus ezAssetDocument::RetrieveAssetInfo(const char* szPlatform)
{
  ezString sPlatform = szPlatform;

  if (sPlatform.IsEmpty())
    sPlatform = ezAssetCurator::GetInstance()->GetActivePlatform();

  ezStatus stat = InternalRetrieveAssetInfo(sPlatform);

  if (stat.m_Result.Succeeded())
  {
    AssetEvent e;
    e.m_Type = AssetEvent::Type::AssetInfoChanged;
    m_AssetEvents.Broadcast(e);
  }

  return stat;
}

void ezAssetDocument::SaveThumbnail(const ezImage& img)
{
  const ezStringBuilder sResourceFile = static_cast<ezAssetDocumentManager*>(GetDocumentManager())->GenerateResourceThumbnailPath(GetDocumentPath());

  EZ_LOG_BLOCK("Save Asset Thumbnail", sResourceFile.GetData());

  ezImage converted;

  // make sure the thumbnail is in a format that Qt understands

  /// \todo A conversion to B8G8R8X8_UNORM currently fails

  if (ezImageConversionBase::Convert(img, converted, ezImageFormat::B8G8R8A8_UNORM).Failed())
  {
    ezLog::Error("Could not convert asset thumbnail to target format: '%s'", sResourceFile.GetData());
    return;
  }

  /// \todo Fix format once ezImage is fixed
  QImage qimg(converted.GetPixelPointer<ezUInt8>(), converted.GetWidth(), converted.GetHeight(), QImage::Format_RGBA8888);

  if (converted.GetWidth() == converted.GetHeight())
  {
    // if necessary scale the image to the proper size
    if (converted.GetWidth() != 256)
      qimg = qimg.scaled(256, 256, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
  }
  else
  {
    // center the image in a square canvas

    // scale the longer edge to 256
    if (converted.GetWidth() > converted.GetHeight())
      qimg = qimg.scaledToWidth(256, Qt::TransformationMode::SmoothTransformation);
    else
      qimg = qimg.scaledToHeight(256, Qt::TransformationMode::SmoothTransformation);

    /// \todo Fix format once ezImage is fixed

    // create a black canvas
    QImage img2(256, 256, QImage::Format_RGBA8888);
    img2.fill(Qt::GlobalColor::black);

    QPoint destPos = QPoint((256 - qimg.width()) / 2, (256 - qimg.height()) / 2);

    // paint the smaller image such that it ends up centered
    QPainter painter(&img2);
    painter.drawImage(destPos, qimg);
    painter.end();

    qimg = img2;
  }

  // make sure the directory exists, Qt will not create sub-folders
  const ezStringBuilder sDir = sResourceFile.GetFileDirectory();
  ezOSFile::CreateDirectoryStructure(sDir);

  // save to JPEG
  if (!qimg.save(QString::fromUtf8(sResourceFile.GetData()), nullptr, 90))
  {
    ezLog::Error("Could not save asset thumbnail: '%s'", sResourceFile.GetData());
    return;
  }

  QtImageCache::InvalidateCache(sResourceFile);
}

