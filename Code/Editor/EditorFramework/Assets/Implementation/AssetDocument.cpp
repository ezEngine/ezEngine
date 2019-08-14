#include <EditorFrameworkPCH.h>

#include <Core/Assets/AssetFileHeader.h>
#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/Assets/AssetDocumentManager.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Texture/Image/ImageConversion.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Utilities/Progress.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <QPainter>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAssetDocument::ezAssetDocument(const char* szDocumentPath, ezDocumentObjectManager* pObjectManager, ezAssetDocEngineConnection engineConnectionType)
    : ezDocument(szDocumentPath, pObjectManager)
{
  m_EngineConnectionType = engineConnectionType;
  m_EngineStatus = (m_EngineConnectionType != ezAssetDocEngineConnection::None) ? EngineStatus::Disconnected : EngineStatus::Unsupported;
  m_pEngineConnection = nullptr;

  if (m_EngineConnectionType != ezAssetDocEngineConnection::None)
  {
    ezEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(
        ezMakeDelegate(&ezAssetDocument::EngineConnectionEventHandler, this));
  }
}

ezAssetDocument::~ezAssetDocument()
{
  m_Mirror.DeInit();

  if (m_EngineConnectionType != ezAssetDocEngineConnection::None)
  {
    ezEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(
        ezMakeDelegate(&ezAssetDocument::EngineConnectionEventHandler, this));

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

ezTaskGroupID ezAssetDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  ezAssetDocumentInfo* pInfo = static_cast<ezAssetDocumentInfo*>(m_pDocumentInfo);

  pInfo->m_AssetTransformDependencies.Clear();
  pInfo->m_RuntimeDependencies.Clear();
  pInfo->m_Outputs.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetTypeName.Assign(QueryAssetType());
  pInfo->ClearMetaData();
  UpdateAssetDocumentInfo(pInfo);

  // In case someone added an empty reference.
  pInfo->m_AssetTransformDependencies.Remove(ezString());
  pInfo->m_RuntimeDependencies.Remove(ezString());

  return ezDocument::InternalSaveDocument(callback);
}

void ezAssetDocument::InternalAfterSaveDocument()
{
  const auto flags = GetAssetFlags();
  ezAssetCurator::GetSingleton()->NotifyOfFileChange(GetDocumentPath());

  // If we request an engine connection but the mirror is not set up yet we are still
  // creating the document and TransformAsset will most likely fail.
  if (m_EngineConnectionType != ezAssetDocEngineConnection::None && m_Mirror.GetIPC())
  {
    if (flags.IsAnySet(ezAssetDocumentFlags::AutoTransformOnSave))
    {
      /// \todo Should only be done for platform agnostic assets
      auto ret = ezAssetCurator::GetSingleton()->TransformAsset(GetGuid(), ezTransformFlags::None);

      if (ret.m_Result.Failed())
      {
        ezLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, GetDocumentPath());
      }
      else
      {
        ezAssetCurator::GetSingleton()->WriteAssetTables();
      }
    }
  }
}

void ezAssetDocument::InitializeAfterLoadingAndSaving()
{
  if (m_EngineConnectionType != ezAssetDocEngineConnection::None)
  {
    m_pEngineConnection = ezEditorEngineProcessConnection::GetSingleton()->CreateEngineConnection(this);
    m_EngineStatus = EngineStatus::Initializing;

    if (m_EngineConnectionType == ezAssetDocEngineConnection::FullObjectMirroring)
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
    if (pChild->GetParentPropertyType()->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;
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
    if (pProp->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;

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
    if (pChild->GetParentPropertyType()->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;

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
  ezUInt64 uiHash = ezHashingUtils::xxHash64(&m_pDocumentInfo->m_DocumentID, sizeof(ezUuid));
  for (auto pChild : GetObjectManager()->GetRootObject()->GetChildren())
  {
    if (pChild->GetParentPropertyType()->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;
    GetChildHash(pChild, uiHash);
    InternalGetMetaDataHash(pChild, uiHash);
  }

  // Gather used types, sort by name to make it table and hash their data
  ezSet<const ezRTTI*> types;
  ezToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
  ezDynamicArray<const ezRTTI*> typesSorted;
  typesSorted.Reserve(types.GetCount());
  for (const ezRTTI* pType : types)
  {
    typesSorted.PushBack(pType);
  }
  typesSorted.Sort([](const ezRTTI* a, const ezRTTI* b) { return ezStringUtils::Compare(a->GetTypeName(), b->GetTypeName()) < 0; });
  for (const ezRTTI* pType : typesSorted)
  {
    uiHash = ezHashingUtils::xxHash64(pType->GetTypeName(), std::strlen(pType->GetTypeName()), uiHash);
    const ezUInt32 uiType = pType->GetTypeVersion();
    uiHash = ezHashingUtils::xxHash64(&uiType, sizeof(uiType), uiHash);
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

ezStatus ezAssetDocument::DoTransformAsset(const ezPlatformProfile* pAssetProfile0 /*= nullptr*/, ezBitflags<ezTransformFlags> transformFlags)
{
  const auto flags = GetAssetFlags();

  if (flags.IsAnySet(ezAssetDocumentFlags::DisableTransform))
    return ezStatus("Asset transform has been disabled on this asset");

  const ezPlatformProfile* pAssetProfile = ezAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);

  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezAssetInfo::TransformState state =
      ezAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), pAssetProfile, GetDocumentTypeDescriptor(), uiHash, uiThumbHash);
  if (state == ezAssetInfo::TransformState::UpToDate && !transformFlags.IsSet(ezTransformFlags::ForceTransform))
    return ezStatus(EZ_SUCCESS, "Transformed asset is already up to date");

  if (uiHash == 0)
    return ezStatus("Computing the hash for this asset or any dependency failed");

  // Write resource
  {
    ezAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(uiHash, GetAssetTypeVersion());
    const auto& outputs = GetAssetDocumentInfo()->m_Outputs;

    auto GenerateOutput = [this, pAssetProfile, &AssetHeader, transformFlags](const char* szOutputTag) -> ezStatus {
      const ezString sTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(GetDocumentPath(), szOutputTag, pAssetProfile);
      auto ret = InternalTransformAsset(sTargetFile, szOutputTag, pAssetProfile, AssetHeader, transformFlags);

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

ezStatus ezAssetDocument::TransformAsset(ezBitflags<ezTransformFlags> transformFlags, const ezPlatformProfile* pAssetProfile)
{
  EZ_PROFILE_SCOPE("TransformAsset");
  if (!transformFlags.IsSet(ezTransformFlags::ForceTransform))
  {
    if (IsModified())
    {
      auto res = SaveDocument().m_Result;
      if (res.Failed())
        return ezStatus(res);
    }

    const auto flags = GetAssetFlags();
    {
      if (flags.IsSet(ezAssetDocumentFlags::DisableTransform) ||
          (flags.IsSet(ezAssetDocumentFlags::OnlyTransformManually) && !transformFlags.IsSet(ezTransformFlags::TriggeredManually)))
        return ezStatus(EZ_SUCCESS, "Transform is disabled for this asset");
    }
  }

  const ezStatus res = DoTransformAsset(pAssetProfile, transformFlags);

  if (transformFlags.IsAnySet(ezTransformFlags::TriggeredManually))
  {
    // some assets modify the document during transformation
    // make sure the state is saved, at least when the user actively executed the action
    SaveDocument();
  }
  return res;
}

ezStatus ezAssetDocument::CreateThumbnail()
{
  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  if (ezAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), ezAssetCurator::GetSingleton()->GetActiveAssetProfile(),
                                                      GetDocumentTypeDescriptor(), uiHash,
                                                      uiThumbHash) ==
      ezAssetInfo::TransformState::UpToDate)
    return ezStatus(EZ_SUCCESS, "Transformed asset is already up to date");

  if (uiHash == 0)
    return ezStatus("Computing the hash for this asset or any dependency failed");

  {
    ThumbnailInfo ThumbnailInfo;
    ThumbnailInfo.SetFileHashAndVersion(uiThumbHash, GetAssetTypeVersion());
    ezStatus res = InternalCreateThumbnail(ThumbnailInfo);

    InvalidateAssetThumbnail();
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
}

ezStatus ezAssetDocument::InternalTransformAsset(const char* szTargetFile, const char* szOutputTag, const ezPlatformProfile* pAssetProfile,
                                                 const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezDeferredFileWriter file;
  file.SetOutput(szTargetFile);

  AssetHeader.Write(file);

  EZ_SUCCEED_OR_RETURN(InternalTransformAsset(file, szOutputTag, pAssetProfile, AssetHeader, transformFlags));

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

ezStatus ezAssetDocument::SaveThumbnail(const ezImage& img, const ThumbnailInfo& thumbnailInfo) const
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

  return SaveThumbnail(qimg, thumbnailInfo);
}

ezStatus ezAssetDocument::SaveThumbnail(const QImage& qimg0, const ThumbnailInfo& thumbnailInfo) const
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

  AppendThumbnailInfo(sResourceFile, thumbnailInfo);
  InvalidateAssetThumbnail();

  return ezStatus(EZ_SUCCESS);
}

void ezAssetDocument::AppendThumbnailInfo(const char* szThumbnailFile, const ThumbnailInfo& thumbnailInfo) const
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

  thumbnailInfo.Serialize(writer);

  if (writer.Close().Failed())
  {
    ezLog::Error("Could not open file for writing: '{0}'", szThumbnailFile);
  }
}

ezStatus ezAssetDocument::RemoteExport(const ezAssetFileHeader& header, const char* szOutputTarget) const
{
  ezProgressRange range("Exporting Asset", 2, false);

  ezLog::Info("Exporting {0} to \"{1}\"", QueryAssetType(), szOutputTarget);

  if (GetEngineStatus() == ezAssetDocument::EngineStatus::Disconnected)
  {
    return ezStatus(ezFmt("Exporting {0} to \"{1}\" failed, engine not started or crashed.", QueryAssetType(), szOutputTarget));
  }
  else if (GetEngineStatus() == ezAssetDocument::EngineStatus::Initializing)
  {
    if (ezEditorEngineProcessConnection::GetSingleton()
            ->WaitForDocumentMessage(GetGuid(), ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), ezTime::Seconds(10))
            .Failed())
    {
      return ezStatus(ezFmt("Exporting {0} to \"{1}\" failed, document initialization timed out.", QueryAssetType(), szOutputTarget));
    }
    EZ_ASSERT_DEV(GetEngineStatus() == ezAssetDocument::EngineStatus::Loaded,
                  "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  range.BeginNextStep(szOutputTarget);

  ezExportDocumentMsgToEngine msg;
  msg.m_sOutputFile = szOutputTarget;
  msg.m_uiAssetHash = header.GetFileHash();
  msg.m_uiVersion = header.GetFileVersion();

  GetEditorEngineConnection()->SendMessage(&msg);

  bool bSuccess = false;
  ezProcessCommunicationChannel::WaitForMessageCallback callback = [&bSuccess](ezProcessMessage* pMsg) -> bool {
    ezExportDocumentMsgToEditor* pMsg2 = ezDynamicCast<ezExportDocumentMsgToEditor*>(pMsg);
    bSuccess = pMsg2->m_bOutputSuccess;
    return true;
  };

  if (ezEditorEngineProcessConnection::GetSingleton()
          ->WaitForDocumentMessage(GetGuid(), ezExportDocumentMsgToEditor::GetStaticRTTI(), ezTime::Seconds(60), &callback)
          .Failed())
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

ezStatus ezAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezStatus("Not implemented");
}

ezStatus ezAssetDocument::RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo) const
{
  ezAssetCurator::GetSingleton()->WriteAssetTables();

  ezLog::Info("Create {0} thumbnail for \"{1}\"", QueryAssetType(), GetDocumentPath());

  if (GetEngineStatus() == ezAssetDocument::EngineStatus::Disconnected)
  {
    return ezStatus(ezFmt("Create {0} thumbnail for \"{1}\" failed, engine not started or crashed.", QueryAssetType(), GetDocumentPath()));
  }
  else if (GetEngineStatus() == ezAssetDocument::EngineStatus::Initializing)
  {
    if (ezEditorEngineProcessConnection::GetSingleton()
            ->WaitForDocumentMessage(GetGuid(), ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), ezTime::Seconds(10))
            .Failed())
    {
      return ezStatus(
          ezFmt("Create {0} thumbnail for \"{1}\" failed, document initialization timed out.", QueryAssetType(), GetDocumentPath()));
    }
    EZ_ASSERT_DEV(GetEngineStatus() == ezAssetDocument::EngineStatus::Loaded,
                  "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  ezCreateThumbnailMsgToEngine msg;
  GetEditorEngineConnection()->SendMessage(&msg);

  ezDataBuffer data;
  ezProcessCommunicationChannel::WaitForMessageCallback callback = [&data](ezProcessMessage* pMsg) -> bool {
    ezCreateThumbnailMsgToEditor* pThumbnailMsg = ezDynamicCast<ezCreateThumbnailMsgToEditor*>(pMsg);
    data = pThumbnailMsg->m_ThumbnailData;
    return true;
  };

  if (ezEditorEngineProcessConnection::GetSingleton()
          ->WaitForDocumentMessage(GetGuid(), ezCreateThumbnailMsgToEditor::GetStaticRTTI(), ezTime::Seconds(60), &callback)
          .Failed())
  {
    return ezStatus(ezFmt("Create {0} thumbnail for \"{1}\" failed timed out.", QueryAssetType(), GetDocumentPath()));
  }
  else
  {
    if (data.GetCount() != msg.m_uiWidth * msg.m_uiHeight * 4)
    {
      return ezStatus(ezFmt("Thumbnail generation for {0} failed, thumbnail data is empty.", QueryAssetType()));
    }

    ezImageHeader imgHeader;
    imgHeader.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
    imgHeader.SetWidth(msg.m_uiWidth);
    imgHeader.SetHeight(msg.m_uiHeight);

    ezImage image;
    image.ResetAndAlloc(imgHeader);
    EZ_ASSERT_DEV(data.GetCount() == imgHeader.ComputeDataSize(), "Thumbnail ezImage has different size than data buffer!");
    ezMemoryUtils::Copy(image.GetPixelPointer<ezUInt8>(), data.GetData(), msg.m_uiWidth * msg.m_uiHeight * 4);
    SaveThumbnail(image, thumbnailInfo);

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
    if (m_EngineConnectionType == ezAssetDocEngineConnection::FullObjectMirroring)
    {
      // make sure the engine clears the document first
      ezDocumentClearMsgToEngine msgClear;
      msgClear.m_DocumentGuid = GetGuid();
      SendMessageToEngine(&msgClear);

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
  pSync->Configure(GetGuid(), [this](ezEditorEngineSyncObject* pSync) { RemoveSyncObject(pSync); });

  m_SyncObjects.PushBack(pSync);
  m_AllSyncObjects[pSync->GetGuid()] = pSync;
}

void ezAssetDocument::RemoveSyncObject(ezEditorEngineSyncObject* pSync) const
{
  m_DeletedObjects.PushBack(pSync->GetGuid());
  m_AllSyncObjects.Remove(pSync->GetGuid());
  m_SyncObjects.RemoveAndSwap(pSync);
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

namespace
{
  static const char* szThumbnailInfoTag = "ezThumb";
}

ezResult ezAssetDocument::ThumbnailInfo::Deserialize(ezStreamReader& Reader)
{
  char tag[8] = {0};

  if(Reader.ReadBytes(tag, 7) != 7)
    return EZ_FAILURE;

  if(!ezStringUtils::IsEqual(tag, szThumbnailInfoTag))
  {
    return EZ_FAILURE;
  }

  Reader >> m_uiHash;
  Reader >> m_uiVersion;
  Reader >> m_uiReserved;

  return EZ_SUCCESS;
}

ezResult ezAssetDocument::ThumbnailInfo::Serialize(ezStreamWriter& Writer) const
{
  Writer.WriteBytes(szThumbnailInfoTag, 7);

  Writer << m_uiHash;
  Writer << m_uiVersion;
  Writer << m_uiReserved;

  return EZ_SUCCESS;
}

