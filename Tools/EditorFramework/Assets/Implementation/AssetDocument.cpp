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
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <EditorFramework/IPC/SyncObject.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentInfo, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_SET_MEMBER_PROPERTY("Dependencies", m_FileDependencies),
    EZ_SET_MEMBER_PROPERTY("References", m_FileReferences),
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

ezAssetDocument::ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager, bool bUseEngineConnection, bool bUseIPCObjectMirror)
  : ezDocument(szDocumentPath, pObjectManager)
{
  m_EngineStatus = bUseEngineConnection ? EngineStatus::Disconnected : EngineStatus::Unsupported;
  m_bUseEngineConnection = bUseEngineConnection;
  m_bUseIPCObjectMirror = bUseIPCObjectMirror;
  m_pEngineConnection = nullptr;

  if (m_bUseEngineConnection)
  {
    ezEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezAssetDocument::EngineConnectionEventHandler, this));
  }
}

ezAssetDocument::~ezAssetDocument()
{
  if (m_EngineStatus == EngineStatus::Initializing)
  {
    // In case we sent the init message, at least wait for the response before dying or someone else after us will see it as 'his' response.
    // TODO: WaitForMessage isn't really a good idea with multiple documents in general. How do you know which response message it was?
    ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), ezTime::Seconds(10));
    EZ_ASSERT_DEV(m_EngineStatus == ezAssetDocument::EngineStatus::Loaded, "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  m_Mirror.DeInit();

  if (m_bUseEngineConnection)
  {
    ezEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetDocument::EngineConnectionEventHandler, this));

    if (m_pEngineConnection)
    {
      ezEditorEngineProcessConnection::GetSingleton()->DestroyEngineConnection(this);
    }
  }
}

ezAssetDocumentManager* ezAssetDocument::GetAssetDocumentManager() const
{
  return static_cast<ezAssetDocumentManager*>(GetDocumentManager());
}

ezBitflags<ezAssetDocumentFlags> ezAssetDocument::GetAssetFlags() const
{
  return GetAssetDocumentManager()->GetAssetDocumentTypeFlags(GetDocumentTypeDescriptor());
}

ezDocumentInfo* ezAssetDocument::CreateDocumentInfo()
{
  return EZ_DEFAULT_NEW(ezAssetDocumentInfo);
}

ezStatus ezAssetDocument::InternalSaveDocument()
{
  ezAssetDocumentInfo* pInfo = static_cast<ezAssetDocumentInfo*>(m_pDocumentInfo);

  pInfo->m_FileDependencies.Clear();
  pInfo->m_FileReferences.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetTypeName = QueryAssetType();

  UpdateAssetDocumentInfo(pInfo);

  // In case someone added an empty reference.
  pInfo->m_FileDependencies.Remove(ezString());
  pInfo->m_FileReferences.Remove(ezString());

  return ezDocument::InternalSaveDocument();
}

void ezAssetDocument::InternalAfterSaveDocument()
{
  const auto flags = GetAssetFlags();

  if (flags.IsAnySet(ezAssetDocumentFlags::AutoTransformOnSave))
  {
    /// \todo Should only be done for platform agnostic assets
    auto ret = ezAssetCurator::GetSingleton()->TransformAsset(GetGuid());

    if (ret.m_Result.Failed())
    {
      ezLog::Error("%s (%s)", ret.m_sMessage.GetData(), GetDocumentPath());
    }
    else
    {
      ezAssetCurator::GetSingleton()->WriteAssetTables();
    }
  }
  ezAssetCurator::GetSingleton()->NotifyOfFileChange(GetDocumentPath());
  ezAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
}

void ezAssetDocument::InitializeAfterLoading()
{
  if (m_bUseEngineConnection)
  {
    m_pEngineConnection = ezEditorEngineProcessConnection::GetSingleton()->CreateEngineConnection(this);
    m_EngineStatus = EngineStatus::Initializing;
    if (m_bUseIPCObjectMirror)
    {
      m_Mirror.SetIPC(m_pEngineConnection);
      m_Mirror.InitSender(GetObjectManager());
    }
  }
}

void ezAssetDocument::AddPrefabDependencies(const ezDocumentObject* pObject, ezAssetDocumentInfo* pInfo) const
{
  {
    const ezDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      pInfo->m_FileDependencies.Insert(ezConversionUtils::ToString(pMeta->m_CreateFromPrefab));
    }

    m_DocumentObjectMetaData.EndReadMetaData();
  }


  const ezHybridArray<ezDocumentObject*, 8>& children = pObject->GetChildren();

  for (auto pChild : children)
  {
    AddPrefabDependencies(pChild, pInfo);
  }
}


void ezAssetDocument::AddReferences(const ezDocumentObject* pObject, ezAssetDocumentInfo* pInfo, bool bInsidePrefab) const
{
  {
    const ezDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData.BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      bInsidePrefab = true;
      pInfo->m_FileReferences.Insert(ezConversionUtils::ToString(pMeta->m_CreateFromPrefab));
    }

    m_DocumentObjectMetaData.EndReadMetaData();
  }

  const ezRTTI* pType = pObject->GetTypeAccessor().GetType();
  ezHybridArray<ezAbstractProperty*, 32> Properties;
  pType->GetAllProperties(Properties);
  for (const auto* pProp : Properties)
  {
    bool bIsReference = pProp->GetAttributeByType<ezAssetBrowserAttribute>() != nullptr;
    bool bIsDependency = pProp->GetAttributeByType<ezFileBrowserAttribute>() != nullptr;
    // add all strings that are marked as asset references or file references
    if (bIsDependency || bIsReference)
    {
      switch (pProp->GetCategory())
      {
      case ezPropertyCategory::Member:
        {
          if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) && pProp->GetSpecificType()->GetVariantType() == ezVariantType::String)
          {
            ezPropertyPath path(pProp->GetPropertyName());
            if (bInsidePrefab && IsDefaultValue(pObject, path, false))
            {
              continue;
            }
            if (bIsDependency)
              pInfo->m_FileDependencies.Insert(pObject->GetTypeAccessor().GetValue(path).Get<ezString>());
            else
              pInfo->m_FileReferences.Insert(pObject->GetTypeAccessor().GetValue(path).Get<ezString>());
          }
        }
        break;
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
        {
          if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) && pProp->GetSpecificType()->GetVariantType() == ezVariantType::String)
          {
            ezPropertyPath path(pProp->GetPropertyName());
            const ezInt32 iCount = pObject->GetTypeAccessor().GetCount(path);

            for (ezInt32 i = 0; i < iCount; ++i)
            {
              ezVariant value = pObject->GetTypeAccessor().GetValue(path, i);
              if (bInsidePrefab && IsDefaultValue(pObject, path, false, i))
              {
                continue;
              }
              if (bIsDependency)
                pInfo->m_FileDependencies.Insert(value.Get<ezString>());
              else
                pInfo->m_FileReferences.Insert(value.Get<ezString>());
            }
          }

        }
        break;
      }
    }

    if (pProp->GetCategory() == ezPropertyCategory::Member && !pProp->GetFlags().IsAnySet(ezPropertyFlags::IsEnum | ezPropertyFlags::Bitflags | ezPropertyFlags::StandardType | ezPropertyFlags::Pointer))
    {
      ezPropertyPath path(pProp->GetPropertyName());
      ezStringBuilder sTemp = ezConversionUtils::ToString(pObject->GetGuid());
      sTemp.Append("/", pProp->GetPropertyName());
      const ezUuid SubObjectGuid = ezUuid::StableUuidForString(sTemp);

      ezDocumentSubObject SubObj(pProp->GetSpecificType());
      SubObj.SetObject(const_cast<ezDocumentObject*>(pObject), path, SubObjectGuid);
      AddReferences(&SubObj, pInfo, bInsidePrefab);
    }
  }

  const ezHybridArray<ezDocumentObject*, 8>& children = pObject->GetChildren();

  for (auto pChild : children)
  {
    AddReferences(pChild, pInfo, bInsidePrefab);
  }
}

void ezAssetDocument::UpdateAssetDocumentInfo(ezAssetDocumentInfo* pInfo) const
{
  const ezDocumentObject* pRoot = GetObjectManager()->GetRootObject();

  AddPrefabDependencies(pRoot, pInfo);
  AddReferences(pRoot, pInfo, false);
}

void ezAssetDocument::EngineConnectionEventHandler(const ezEditorEngineProcessConnection::Event& e)
{
  if (e.m_Type == ezEditorEngineProcessConnection::Event::Type::ProcessCrashed)
  {
    m_EngineStatus = EngineStatus::Disconnected;
  }
  else if (e.m_Type == ezEditorEngineProcessConnection::Event::Type::ProcessStarted)
  {
    m_EngineStatus = EngineStatus::Initializing;
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

  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  if (ezAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), szPlatform, GetDocumentTypeDescriptor(), uiHash, uiThumbHash) == ezAssetInfo::TransformState::UpToDate)
    return ezStatus(EZ_SUCCESS, "Transformed asset is already up to date");

  if (uiHash == 0)
    return ezStatus("Computing the hash for this asset or any dependency failed");

  // Write resource
  {
    ezAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(uiHash, GetAssetTypeVersion());

    const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);
    const ezString sTargetFile = static_cast<ezAssetDocumentManager*>(GetDocumentTypeDescriptor()->m_pManager)->GetFinalOutputFileName(GetDocumentTypeDescriptor(), GetDocumentPath(), sPlatform);

    auto ret = InternalTransformAsset(sTargetFile, sPlatform, AssetHeader);

    // if writing failed, make sure the output file does not exist
    if (ret.m_Result.Failed())
    {
      ezFileSystem::DeleteFile(sTargetFile);
    }

    ezAssetCurator::GetSingleton()->NotifyOfFileChange(sTargetFile);
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
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
  }

  return TransformAssetManually(szPlatform);
}

ezStatus ezAssetDocument::CreateThumbnail()
{
  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  if (ezAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), nullptr, GetDocumentTypeDescriptor(), uiHash, uiThumbHash) == ezAssetInfo::TransformState::UpToDate)
    return ezStatus("Transformed asset is already up to date");

  if (uiHash == 0)
    return ezStatus("Computing the hash for this asset or any dependency failed");

  {
    ezAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(uiThumbHash, GetAssetTypeVersion());
    ezStatus res =  InternalCreateThumbnail(AssetHeader);

    InvalidateAssetThumbnail();
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
}

ezStatus ezAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szPlatform, const ezAssetFileHeader& AssetHeader)
{
  ezDeferredFileWriter file;

  file.SetOutput(szTargetFile);

  AssetHeader.Write(file);

  auto ret = InternalTransformAsset(file, szPlatform, AssetHeader);

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
  const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);

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

void ezAssetDocument::InvalidateAssetThumbnail() const
{
  const ezStringBuilder sResourceFile = GetThumbnailFilePath();
  ezAssetCurator::GetSingleton()->NotifyOfFileChange(sResourceFile);
  ezQtImageCache::GetSingleton()->InvalidateCache(sResourceFile);
}

ezStatus ezAssetDocument::SaveThumbnail(const ezImage& img, const ezAssetFileHeader& header) const
{
  ezImage converted;

  // make sure the thumbnail is in a format that Qt understands

  /// \todo A conversion to B8G8R8X8_UNORM currently fails

  if (ezImageConversion::Convert(img, converted, ezImageFormat::R8G8B8A8_UNORM).Failed())
  {
    const ezStringBuilder sResourceFile = GetThumbnailFilePath();

    ezLog::Error("Could not convert asset thumbnail to target format: '%s'", sResourceFile.GetData());
    return ezStatus("Could not convert asset thumbnail to target format: '%s'", sResourceFile.GetData());
  }

  QImage qimg(converted.GetPixelPointer<ezUInt8>(), converted.GetWidth(), converted.GetHeight(), QImage::Format_RGBA8888);

  return SaveThumbnail(qimg, header);
}

ezStatus ezAssetDocument::SaveThumbnail(const QImage& qimg0, const ezAssetFileHeader& header) const
{
  const ezStringBuilder sResourceFile = GetThumbnailFilePath();
  EZ_LOG_BLOCK("Save Asset Thumbnail", sResourceFile.GetData());

  QImage qimg = qimg0;

  if (qimg.width() == qimg.height())
  {
    // if necessary scale the image to the proper size
    if (qimg.width() != 256)
      qimg = qimg.scaled(256, 256, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
  }
  else
  {
    // center the image in a square canvas

    // scale the longer edge to 256
    if (qimg.width() > qimg.height())
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
    return ezStatus("Could not save asset thumbnail: '%s'", sResourceFile.GetData());
  }

  AppendThumbnailInfo(sResourceFile, header);
  InvalidateAssetThumbnail();

  return ezStatus(EZ_SUCCESS);
}

void ezAssetDocument::AppendThumbnailInfo(const char* szThumbnailFile, const ezAssetFileHeader& header) const
{
  ezMemoryStreamStorage storage;
  {
    ezFileReader reader;
    if (reader.Open(szThumbnailFile).Failed())
    {
      return;
    }
    storage.ReadAll(reader);
  }

  ezDeferredFileWriter writer;
  writer.SetOutput(szThumbnailFile);
  writer.WriteBytes(storage.GetData(), storage.GetStorageSize());

  static const char* sztTag = "ezThumb";
  writer.WriteBytes(sztTag, 7);
  header.Write(writer);

  if (writer.Close().Failed())
  {
    ezLog::Error("Could not open file for writing: '%s'", szThumbnailFile);
  }
}

ezString ezAssetDocument::GetDocumentPathFromGuid(const ezUuid& documentGuid) const
{
  ezAssetCurator::ezLockedAssetInfo pAssetInfo = ezAssetCurator::GetSingleton()->GetAssetInfo2(documentGuid);

  return pAssetInfo->m_sAbsolutePath;
}

ezStatus ezAssetDocument::RemoteExport(const ezAssetFileHeader& header, const char* szOutputTarget) const
{
  ezLog::Info("Exporting %s to \"%s\"", QueryAssetType(), szOutputTarget);

  if (GetEngineStatus() == ezAssetDocument::EngineStatus::Disconnected)
  {
    return ezStatus("Exporting %s to \"%s\" failed, engine not started or crashed.", QueryAssetType(), szOutputTarget);
  }
  else if (GetEngineStatus() == ezAssetDocument::EngineStatus::Initializing)
  {
    if (ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), ezTime::Seconds(10)).Failed())
    {
      return ezStatus("Exporting %s to \"%s\" failed, document initialization timed out.", QueryAssetType(), szOutputTarget);
    }
    EZ_ASSERT_DEV(GetEngineStatus() == ezAssetDocument::EngineStatus::Loaded, "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  ezExportDocumentMsgToEngine msg;
  msg.m_sOutputFile = szOutputTarget;
  msg.m_uiAssetHash = header.GetFileHash();
  msg.m_uiVersion = header.GetFileVersion();

  GetEditorEngineConnection()->SendMessage(&msg);

  bool bSuccess = false;
  ezProcessCommunication::WaitForMessageCallback callback = [&bSuccess](ezProcessMessage* pMsg)
  {
    ezExportDocumentMsgToEditor* pMsg2 = ezDynamicCast<ezExportDocumentMsgToEditor*>(pMsg);
    bSuccess = pMsg2->m_bOutputSuccess;
  };

  if (ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezExportDocumentMsgToEditor::GetStaticRTTI(), ezTime::Seconds(60), &callback).Failed())
  {
    return ezStatus("Remote exporting %s to \"%s\" timed out.", QueryAssetType(), msg.m_sOutputFile.GetData());
  }
  else
  {
    if (!bSuccess)
    {
      return ezStatus("Remote exporting %s to \"%s\" failed.", QueryAssetType(), msg.m_sOutputFile.GetData());
    }

    ezLog::Success("%s \"%s\" has been exported.", QueryAssetType(), msg.m_sOutputFile.GetData());

    ShowDocumentStatus("%s exported successfully", QueryAssetType());

    return ezStatus(EZ_SUCCESS);
  }
}

ezStatus ezAssetDocument::InternalCreateThumbnail(const ezAssetFileHeader& AssetHeader)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezStatus("Not implemented");
}

ezStatus ezAssetDocument::RemoteCreateThumbnail(const ezAssetFileHeader& header) const
{
  ezAssetCurator::GetSingleton()->WriteAssetTables();

  ezLog::Info("Create %s thumbnail for \"%s\"", QueryAssetType(), GetDocumentPath());

  if (GetEngineStatus() == ezAssetDocument::EngineStatus::Disconnected)
  {
    return ezStatus("Create %s thumbnail for \"%s\" failed, engine not started or crashed.", QueryAssetType(), GetDocumentPath());
  }
  else if (GetEngineStatus() == ezAssetDocument::EngineStatus::Initializing)
  {
    if (ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), ezTime::Seconds(10)).Failed())
    {
      return ezStatus("Create %s thumbnail for \"%s\" failed, document initialization timed out.", QueryAssetType(), GetDocumentPath());
    }
    EZ_ASSERT_DEV(GetEngineStatus() == ezAssetDocument::EngineStatus::Loaded, "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  ezCreateThumbnailMsgToEngine msg;
  GetEditorEngineConnection()->SendMessage(&msg);

  ezDataBuffer data;
  ezProcessCommunication::WaitForMessageCallback callback = [&data](ezProcessMessage* pMsg)
  {
    ezCreateThumbnailMsgToEditor* pThumbnailMsg = ezDynamicCast<ezCreateThumbnailMsgToEditor*>(pMsg);
    data = pThumbnailMsg->m_ThumbnailData;
  };

  if (ezEditorEngineProcessConnection::GetSingleton()->WaitForMessage(ezCreateThumbnailMsgToEditor::GetStaticRTTI(), ezTime::Seconds(60), &callback).Failed())
  {
    return ezStatus("Create %s thumbnail for \"%s\" failed timed out.", QueryAssetType(), GetDocumentPath());
  }
  else
  {
    if (data.GetCount() != msg.m_uiWidth * msg.m_uiHeight * 4)
    {
      return ezStatus("Thumbnail generation for %s failed, thumbnail data is empty.", QueryAssetType());
    }

    ezImage image;
    image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
    image.SetWidth(msg.m_uiWidth);
    image.SetHeight(msg.m_uiHeight);
    image.AllocateImageData();
    EZ_ASSERT_DEV(data.GetCount() == image.GetDataSize(), "Thumbnail ezImage has different size than data buffer!");
    ezMemoryUtils::Copy(image.GetDataPointer<ezUInt8>(), data.GetData(), msg.m_uiWidth * msg.m_uiHeight * 4);
    SaveThumbnail(image, header);

    ezLog::Success("%s thumbnail for \"%s\" has been exported.", QueryAssetType(), GetDocumentPath());

    ShowDocumentStatus("%s thumbnail created successfully", QueryAssetType());

    return ezStatus(EZ_SUCCESS);
  }
}

ezUInt16 ezAssetDocument::GetAssetTypeVersion() const
{
  return (ezUInt16)GetDynamicRTTI()->GetTypeVersion();
}

ezString ezAssetDocument::GetFinalOutputFileName(const char* szPlatform /*= nullptr*/) const
{
  const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);
  return GetAssetDocumentManager()->GetFinalOutputFileName(GetDocumentTypeDescriptor(), GetDocumentPath(), szPlatform);
}

void ezAssetDocument::SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage /*= false*/) const
{
  GetEditorEngineConnection()->SendMessage(pMessage);
}

void ezAssetDocument::HandleEngineMessage(const ezEditorEngineDocumentMsg* pMsg)
{
  if (pMsg->GetDynamicRTTI()->IsDerivedFrom<ezDocumentOpenResponseMsgToEditor>())
  {
    if (m_bUseIPCObjectMirror)
    {
      m_Mirror.SendDocument();
    }
    m_EngineStatus = EngineStatus::Loaded;
    // make sure all sync objects are 'modified' so that they will get resent as well
    for (auto* pObject : m_SyncObjects)
    {
      pObject->SetModified();
    }
  }

  m_ProcessMessageEvent.Broadcast(pMsg);
}

void ezAssetDocument::AddSyncObject(ezEditorEngineSyncObject* pSync) const
{
  m_SyncObjects.PushBack(pSync);
  pSync->m_pOwner = this;
  m_AllSyncObjects[pSync->GetGuid()] = pSync;
}

void ezAssetDocument::RemoveSyncObject(ezEditorEngineSyncObject* pSync) const
{
  m_DeletedObjects.PushBack(pSync->GetGuid());
  m_AllSyncObjects.Remove(pSync->GetGuid());
  m_SyncObjects.RemoveSwap(pSync);
  pSync->m_pOwner = nullptr;
}

ezEditorEngineSyncObject* ezAssetDocument::FindSyncObject(const ezUuid& guid) const
{
  ezEditorEngineSyncObject* pSync = nullptr;
  m_AllSyncObjects.TryGetValue(guid, pSync);
  return pSync;
}

void ezAssetDocument::SyncObjectsToEngine()
{
  // Tell the engine which sync objects have been removed recently
  {
    for (const auto& guid : m_DeletedObjects)
    {
      ezEditorEngineSyncObjectMsg msg;
      msg.m_ObjectGuid = guid;
      SendMessageToEngine(&msg);
    }

    m_DeletedObjects.Clear();
  }

  for (auto* pObject : m_SyncObjects)
  {
    if (!pObject->GetModified())
      continue;

    ezEditorEngineSyncObjectMsg msg;
    msg.m_ObjectGuid = pObject->m_SyncObjectGuid;
    msg.m_sObjectType = pObject->GetDynamicRTTI()->GetTypeName();

    ezMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezReflectionSerializer::WriteObjectToBinary(writer, pObject->GetDynamicRTTI(), pObject);
    msg.m_ObjectData = ezArrayPtr<const ezUInt8>(storage.GetData(), storage.GetStorageSize());

    SendMessageToEngine(&msg);

    pObject->SetModified(false);
  }
}

const char* ezAssetDocument::GetDocumentTypeDisplayString() const
{
  static ezStringBuilder dummy; // must be static to survive the function call
  dummy = QueryAssetType();
  dummy.Append(" Asset");

  return dummy;
}
