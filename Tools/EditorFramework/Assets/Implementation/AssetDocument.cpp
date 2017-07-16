#include <PCH.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <Core/Assets/AssetFileHeader.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <Foundation/Image/ImageConversion.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <QPainter>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocumentInfo, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_SET_MEMBER_PROPERTY("Dependencies", m_AssetTransformDependencies),
    EZ_SET_MEMBER_PROPERTY("References", m_RuntimeDependencies),
    EZ_SET_MEMBER_PROPERTY("Outputs", m_Outputs),
    EZ_MEMBER_PROPERTY("Hash", m_uiSettingsHash),
    EZ_ACCESSOR_PROPERTY("AssetType", GetAssetTypeName, SetAssetTypeName),
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

const ezAssetDocumentInfo* ezAssetDocument::GetAssetDocumentInfo() const
{
  return static_cast<ezAssetDocumentInfo*>(m_pDocumentInfo);
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

  pInfo->m_AssetTransformDependencies.Clear();
  pInfo->m_RuntimeDependencies.Clear();
  pInfo->m_Outputs.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetTypeName.Assign(QueryAssetType());

  UpdateAssetDocumentInfo(pInfo);

  // In case someone added an empty reference.
  pInfo->m_AssetTransformDependencies.Remove(ezString());
  pInfo->m_RuntimeDependencies.Remove(ezString());

  return ezDocument::InternalSaveDocument();
}

void ezAssetDocument::InternalAfterSaveDocument()
{
  const auto flags = GetAssetFlags();

  if (flags.IsAnySet(ezAssetDocumentFlags::AutoTransformOnSave))
  {
    /// \todo Should only be done for platform agnostic assets
    auto ret = ezAssetCurator::GetSingleton()->TransformAsset(GetGuid(), false);

    if (ret.m_Result.Failed())
    {
      ezLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, GetDocumentPath());
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
      ezStringBuilder tmp;
      pInfo->m_AssetTransformDependencies.Insert(ezConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
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
      ezStringBuilder tmp;
      pInfo->m_RuntimeDependencies.Insert(ezConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
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
            if (bInsidePrefab && IsDefaultValue(pObject, pProp->GetPropertyName(), false))
            {
              continue;
            }

            const ezVariant& var = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());

            if (var.IsA<ezString>())
            {
              if (bIsDependency)
                pInfo->m_AssetTransformDependencies.Insert(var.Get<ezString>());
              else
                pInfo->m_RuntimeDependencies.Insert(var.Get<ezString>());
            }
          }
        }
        break;
      case ezPropertyCategory::Array:
      case ezPropertyCategory::Set:
        {
          if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType) && pProp->GetSpecificType()->GetVariantType() == ezVariantType::String)
          {
            const ezInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());

            for (ezInt32 i = 0; i < iCount; ++i)
            {
              ezVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
              if (bInsidePrefab && IsDefaultValue(pObject, pProp->GetPropertyName(), false, i))
              {
                continue;
              }
              if (bIsDependency)
                pInfo->m_AssetTransformDependencies.Insert(value.Get<ezString>());
              else
                pInfo->m_RuntimeDependencies.Insert(value.Get<ezString>());
            }
          }

        }
        break;
      }
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
  ezUInt64 uiHash = ezHashing::MurmurHash64(&m_pDocumentInfo->m_DocumentID, sizeof(ezUuid));
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    GetChildHash(pChild, uiHash);
    InternalGetMetaDataHash(pChild, uiHash);
  }

  // Gather used types, sort by name to make it table and hash their data
  ezSet<const ezRTTI*> types;
  ezToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types, false);
  ezDynamicArray<const ezRTTI*> typesSorted;
  typesSorted.Reserve(types.GetCount());
  for (const ezRTTI* pType : types)
  {
    typesSorted.PushBack(pType);
  }
  typesSorted.Sort([](const ezRTTI* a, const ezRTTI* b) { return ezStringUtils::Compare(a->GetTypeName(), b->GetTypeName()) < 0; });
  for (const ezRTTI* pType : typesSorted)
  {
    uiHash = ezHashing::MurmurHash64(pType->GetTypeName(), std::strlen(pType->GetTypeName()), uiHash);
    const ezUInt32 uiType = pType->GetTypeVersion();
    uiHash = ezHashing::MurmurHash64(&uiType, sizeof(uiType), uiHash);
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

ezStatus ezAssetDocument::DoTransformAsset(const char* szPlatform /*= nullptr*/, bool bTriggeredManually)
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
    const auto& outputs = GetAssetDocumentInfo()->m_Outputs;

    auto GenerateOutput = [this, szPlatform, &AssetHeader, bTriggeredManually](const char* szOutputTag) -> ezStatus
    {
      const ezString sPlatform = ezAssetDocumentManager::DetermineFinalTargetPlatform(szPlatform);
      const ezString sTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(GetDocumentPath(), szOutputTag, sPlatform);
      auto ret = InternalTransformAsset(sTargetFile, szOutputTag, sPlatform, AssetHeader, bTriggeredManually);

      // if writing failed, make sure the output file does not exist
      if (ret.m_Result.Failed())
      {
        ezFileSystem::DeleteFile(sTargetFile);
      }
      ezAssetCurator::GetSingleton()->NotifyOfFileChange(sTargetFile);
      return ret;
    };

    ezStatus res(EZ_SUCCESS);
    for (auto it = outputs.GetIterator(); it.IsValid(); ++it)
    {
      res = GenerateOutput(it.Key());
      if (res.Failed())
        return res;
    }

    res = GenerateOutput("");
    if (res.Failed())
      return res;

    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
}

ezStatus ezAssetDocument::TransformAsset(bool bTriggeredManually, const char* szPlatform)
{
  if (!bTriggeredManually)
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
  }

  return DoTransformAsset(szPlatform, bTriggeredManually);
}

ezStatus ezAssetDocument::CreateThumbnail()
{
  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  if (ezAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), nullptr, GetDocumentTypeDescriptor(), uiHash, uiThumbHash) == ezAssetInfo::TransformState::UpToDate)
    return ezStatus(EZ_SUCCESS, "Transformed asset is already up to date");

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

ezStatus ezAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const char* szPlatform, const ezAssetFileHeader& AssetHeader, bool bTriggeredManually)
{
  ezDeferredFileWriter file;
  file.SetOutput(szTargetFile);

  AssetHeader.Write(file);

  EZ_SUCCEED_OR_RETURN(InternalTransformAsset(file, szOutputTag, szPlatform, AssetHeader, bTriggeredManually));

  if (file.Close().Failed())
  {
    ezLog::Error("Could not open file for writing: '{0}'", szTargetFile);
    return ezStatus("Opening the asset output file failed");
  }

  return ezStatus(EZ_SUCCESS);
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

    ezLog::Error("Could not convert asset thumbnail to target format: '{0}'", sResourceFile);
    return ezStatus(ezFmt("Could not convert asset thumbnail to target format: '{0}'", sResourceFile));
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
    ezLog::Error("Could not save asset thumbnail: '{0}'", sResourceFile);
    return ezStatus(ezFmt("Could not save asset thumbnail: '{0}'", sResourceFile));
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
    ezLog::Error("Could not open file for writing: '{0}'", szThumbnailFile);
  }
}

ezStatus ezAssetDocument::RemoteExport(const ezAssetFileHeader& header, const char* szOutputTarget) const
{
  ezLog::Info("Exporting {0} to \"{1}\"", QueryAssetType(), szOutputTarget);

  if (GetEngineStatus() == ezAssetDocument::EngineStatus::Disconnected)
  {
    return ezStatus(ezFmt("Exporting {0} to \"{1}\" failed, engine not started or crashed.", QueryAssetType(), szOutputTarget));
  }
  else if (GetEngineStatus() == ezAssetDocument::EngineStatus::Initializing)
  {
    if (ezEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), ezTime::Seconds(10)).Failed())
    {
      return ezStatus(ezFmt("Exporting {0} to \"{1}\" failed, document initialization timed out.", QueryAssetType(), szOutputTarget));
    }
    EZ_ASSERT_DEV(GetEngineStatus() == ezAssetDocument::EngineStatus::Loaded, "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  ezExportDocumentMsgToEngine msg;
  msg.m_sOutputFile = szOutputTarget;
  msg.m_uiAssetHash = header.GetFileHash();
  msg.m_uiVersion = header.GetFileVersion();

  GetEditorEngineConnection()->SendMessage(&msg);

  bool bSuccess = false;
  ezProcessCommunicationChannel::WaitForMessageCallback callback = [&bSuccess](ezProcessMessage* pMsg)->bool
  {
    ezExportDocumentMsgToEditor* pMsg2 = ezDynamicCast<ezExportDocumentMsgToEditor*>(pMsg);
    bSuccess = pMsg2->m_bOutputSuccess;
    return true;
  };

  if (ezEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), ezExportDocumentMsgToEditor::GetStaticRTTI(), ezTime::Seconds(60), &callback).Failed())
  {
    return ezStatus(ezFmt("Remote exporting {0} to \"{1}\" timed out.", QueryAssetType(), msg.m_sOutputFile));
  }
  else
  {
    if (!bSuccess)
    {
      return ezStatus(ezFmt("Remote exporting {0} to \"{1}\" failed.", QueryAssetType(), msg.m_sOutputFile));
    }

    ezLog::Success("{0} \"{1}\" has been exported.", QueryAssetType(), msg.m_sOutputFile);

    ShowDocumentStatus(ezFmt("{0} exported successfully", QueryAssetType()));

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

  ezLog::Info("Create {0} thumbnail for \"{1}\"", QueryAssetType(), GetDocumentPath());

  if (GetEngineStatus() == ezAssetDocument::EngineStatus::Disconnected)
  {
    return ezStatus(ezFmt("Create {0} thumbnail for \"{1}\" failed, engine not started or crashed.", QueryAssetType(), GetDocumentPath()));
  }
  else if (GetEngineStatus() == ezAssetDocument::EngineStatus::Initializing)
  {
    if (ezEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), ezTime::Seconds(10)).Failed())
    {
      return ezStatus(ezFmt("Create {0} thumbnail for \"{1}\" failed, document initialization timed out.", QueryAssetType(), GetDocumentPath()));
    }
    EZ_ASSERT_DEV(GetEngineStatus() == ezAssetDocument::EngineStatus::Loaded, "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  ezCreateThumbnailMsgToEngine msg;
  GetEditorEngineConnection()->SendMessage(&msg);

  ezDataBuffer data;
  ezProcessCommunicationChannel::WaitForMessageCallback callback = [&data](ezProcessMessage* pMsg)->bool
  {
    ezCreateThumbnailMsgToEditor* pThumbnailMsg = ezDynamicCast<ezCreateThumbnailMsgToEditor*>(pMsg);
    data = pThumbnailMsg->m_ThumbnailData;
    return true;
  };

  if (ezEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), ezCreateThumbnailMsgToEditor::GetStaticRTTI(), ezTime::Seconds(60), &callback).Failed())
  {
    return ezStatus(ezFmt("Create {0} thumbnail for \"{1}\" failed timed out.", QueryAssetType(), GetDocumentPath()));
  }
  else
  {
    if (data.GetCount() != msg.m_uiWidth * msg.m_uiHeight * 4)
    {
      return ezStatus(ezFmt("Thumbnail generation for {0} failed, thumbnail data is empty.", QueryAssetType()));
    }

    ezImage image;
    image.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
    image.SetWidth(msg.m_uiWidth);
    image.SetHeight(msg.m_uiHeight);
    image.AllocateImageData();
    EZ_ASSERT_DEV(data.GetCount() == image.GetDataSize(), "Thumbnail ezImage has different size than data buffer!");
    ezMemoryUtils::Copy(image.GetDataPointer<ezUInt8>(), data.GetData(), msg.m_uiWidth * msg.m_uiHeight * 4);
    SaveThumbnail(image, header);

    ezLog::Success("{0} thumbnail for \"{1}\" has been exported.", QueryAssetType(), GetDocumentPath());

    ShowDocumentStatus(ezFmt("{0} thumbnail created successfully", QueryAssetType()));

    return ezStatus(EZ_SUCCESS);
  }
}

ezUInt16 ezAssetDocument::GetAssetTypeVersion() const
{
  return (ezUInt16)GetDynamicRTTI()->GetTypeVersion();
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
      // this is currently too problematic
      // make sure the engine clears the document first
      //ezDocumentClearMsgToEngine msgClear;
      //msgClear.m_DocumentGuid = GetGuid();
      //SendMessageToEngine(&msgClear);

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
  pSync->Configure(GetGuid(), [this](ezEditorEngineSyncObject* pSync)
  {
    RemoveSyncObject(pSync);
  });

  m_SyncObjects.PushBack(pSync);
  m_AllSyncObjects[pSync->GetGuid()] = pSync;
}

void ezAssetDocument::RemoveSyncObject(ezEditorEngineSyncObject* pSync) const
{
  m_DeletedObjects.PushBack(pSync->GetGuid());
  m_AllSyncObjects.Remove(pSync->GetGuid());
  m_SyncObjects.RemoveSwap(pSync);
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
