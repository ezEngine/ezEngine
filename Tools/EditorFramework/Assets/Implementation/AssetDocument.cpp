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
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentInfo, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_ACCESSOR_PROPERTY("Dependencies", GetDependencies, SetDependencies),
    //EZ_ACCESSOR_PROPERTY("References", GetReferences, SetReferences),
    EZ_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
    EZ_MEMBER_PROPERTY("AssetType", m_sAssetTypeName),
  }
  EZ_END_PROPERTIES
}
EZ_END_DYNAMIC_REFLECTED_TYPE

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE

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

  ezHybridArray<ezString, 16> temp;
  sTemp.Split(false, temp, ";");

  for (const auto& s : temp)
    m_FileDependencies.Insert(s);
}


//ezString ezAssetDocumentInfo::GetReferences() const
//{
//  ezStringBuilder s;
//
//  for (const auto& dep : m_FileReferences)
//  {
//    s.AppendFormat("%s;", dep.GetData());
//  }
//
//  s.Shrink(0, 1);
//  return s;
//}
//
//void ezAssetDocumentInfo::SetReferences(ezString s)
//{
//  m_FileReferences.Clear();
//
//  ezStringBuilder sTemp = s;
//
//  ezHybridArray<ezString, 16> temp;
//  sTemp.Split(false, temp, ";");
//
//  for (const auto& s : temp)
//    m_FileReferences.Insert(s);
//}


ezAssetDocument::ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager)
  : ezDocument(szDocumentPath, pObjectManager)
{
}

ezAssetDocument::~ezAssetDocument()
{
}

ezDocumentInfo* ezAssetDocument::CreateDocumentInfo()
{
  return EZ_DEFAULT_NEW(ezAssetDocumentInfo);
}

ezString ezAssetDocument::DetermineFinalTargetPlatform(const char* szPlatform)
{
  if (ezStringUtils::IsNullOrEmpty(szPlatform))
  {
    return ezAssetCurator::GetSingleton()->GetActivePlatform();
  }

  return szPlatform;
}

ezStatus ezAssetDocument::InternalSaveDocument()
{
  ezAssetDocumentInfo* pInfo = static_cast<ezAssetDocumentInfo*>(m_pDocumentInfo);

  pInfo->m_FileDependencies.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetTypeName = QueryAssetType();

  UpdateAssetDocumentInfo(pInfo);

  return ezDocument::InternalSaveDocument();
}

void ezAssetDocument::InternalAfterSaveDocument()
{
  if (GetAssetFlags().IsAnySet(ezAssetDocumentFlags::AutoTransformOnSave))
  {
    /// \todo Should only be done for platform agnostic assets

    auto ret = TransformAsset();

    if (ret.m_Result.Failed())
    {
      ezLog::Error("%s (%s)", ret.m_sMessage.GetData(), GetDocumentPath());
    }
    else
    {
      ezAssetCurator::GetSingleton()->WriteAssetTables();
    }
  }
}

ezUInt64 ezAssetDocument::GetDocumentHash() const
{
  ezUInt64 uiHash = 0;
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    GetChildHash(pChild, uiHash);
    InternalGetMetaDataHash(pChild, uiHash);
  }
  return uiHash;
}

void ezAssetDocument::GetChildHash(const ezDocumentObject* pObject, ezUInt64& uiHash) const
{
  pObject->ComputeObjectHash(uiHash);

  for (auto pChild : pObject->GetChildren())
  {
    GetChildHash(pChild, uiHash);
  }
}

ezStatus ezAssetDocument::TransformAssetManually(const char* szPlatform /*= nullptr*/)
{
  const auto flags = GetAssetFlags();

  if (flags.IsAnySet(ezAssetDocumentFlags::DisableTransform))
    return ezStatus("Asset transform has been disabled on this asset");

  // cannot be transformed without a window, so skip this
  if (flags.IsAnySet(ezAssetDocumentFlags::TransformRequiresWindow) && !HasWindowBeenRequested())
    return ezStatus("Asset cannot be transformed without an open document window");

  const ezUInt64 uiHash = ezAssetCurator::GetSingleton()->GetAssetDependencyHash(GetGuid());

  if (uiHash == 0)
    return ezStatus("Computing the hash for this asset or any dependency failed");

  const ezString sPlatform = DetermineFinalTargetPlatform(szPlatform);
  const ezString sTargetFile = GetFinalOutputFileName(sPlatform);

  if (ezAssetDocumentManager::IsResourceUpToDate(uiHash, GetAssetTypeVersion(), sTargetFile))
    return ezStatus(EZ_SUCCESS, "Transformed asset is already up to date");

  // Write resource
  {
    ezAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(uiHash, GetAssetTypeVersion());

    auto ret = InternalTransformAsset(sTargetFile, sPlatform, AssetHeader);

    // if writing failed, make sure the output file does not exist
    if (ret.m_Result.Failed())
    {
      ezFileSystem::DeleteFile(sTargetFile);
    }

    return ret;
  }
}

ezStatus ezAssetDocument::TransformAsset(const char* szPlatform)
{
  if (IsModified())
  {
    auto res = SaveDocument().m_Result;
    if (res.Failed())
      return ezStatus(res);
  }

  const auto flags = GetAssetFlags();
  {
    if (flags.IsAnySet(ezAssetDocumentFlags::DisableTransform | ezAssetDocumentFlags::OnlyTransformManually))
      return ezStatus(EZ_SUCCESS, "Transform is disabled for this asset");

    // cannot be transformed without a window, so skip this
    if (flags.IsAnySet(ezAssetDocumentFlags::TransformRequiresWindow) && !HasWindowBeenRequested())
      return ezStatus(EZ_SUCCESS, "Transform requires a document window, which is currently not available");
  }

  return TransformAssetManually(szPlatform);
}

ezStatus ezAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  ezDeferredFileWriter file;

  file.SetOutput(szTargetFile);

  AssetHeader.Write(file);

  auto ret = InternalTransformAsset(file, szPlatform);

  if (ret.m_Result.Failed())
    return ret;

  if (file.Close().Failed())
  {
    ezLog::Error("Could not open file for writing: '%s'", szTargetFile);
    return ezStatus("Opening the asset output file failed");
  }

  return ezStatus(EZ_SUCCESS);
}

ezStatus ezAssetDocument::RetrieveAssetInfo(const char* szPlatform)
{
  const ezString sPlatform = DetermineFinalTargetPlatform(szPlatform);

  ezStatus stat = InternalRetrieveAssetInfo(sPlatform);

  if (stat.m_Result.Succeeded())
  {
    AssetEvent e;
    e.m_Type = AssetEvent::Type::AssetInfoChanged;
    m_AssetEvents.Broadcast(e);
  }

  return stat;
}

ezString ezAssetDocument::GetThumbnailFilePath() const
{
  return static_cast<ezAssetDocumentManager*>(GetDocumentManager())->GenerateResourceThumbnailPath(GetDocumentPath());
}

void ezAssetDocument::InvalidateAssetThumbnail()
{
  const ezStringBuilder sResourceFile = GetThumbnailFilePath();
  ezQtImageCache::InvalidateCache(sResourceFile);
}

void ezAssetDocument::SaveThumbnail(const ezImage& img)
{
  const ezStringBuilder sResourceFile = GetThumbnailFilePath();

  EZ_LOG_BLOCK("Save Asset Thumbnail", sResourceFile.GetData());

  ezImage converted;

  // make sure the thumbnail is in a format that Qt understands

  /// \todo A conversion to B8G8R8X8_UNORM currently fails

  if (ezImageConversion::Convert(img, converted, ezImageFormat::B8G8R8A8_UNORM).Failed())
  {
    ezLog::Error("Could not convert asset thumbnail to target format: '%s'", sResourceFile.GetData());
    return;
  }

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

  InvalidateAssetThumbnail();
}

ezString ezAssetDocument::GetFinalOutputFileName(const char* szPlatform)
{
  const ezString sPlatform = DetermineFinalTargetPlatform(szPlatform);

  return static_cast<ezAssetDocumentManager*>(GetDocumentManager())->GenerateResourceFileName(GetDocumentPath(), sPlatform);
}

ezBitflags<ezAssetDocumentFlags> ezAssetDocument::GetAssetFlags() const
{
  return ezAssetDocumentFlags::Default;
}

