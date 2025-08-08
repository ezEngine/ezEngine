#include <EditorFramework/EditorFrameworkPCH.h>

#include <EditorEngineProcessFramework/IPC/SyncObject.h>
#include <EditorFramework/Assets/AssetCurator.h>
#include <EditorFramework/Assets/AssetDocument.h>
#include <EditorFramework/EditorApp/EditorApp.moc.h>
#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/OSFile.h>
#include <Foundation/Serialization/ReflectionSerializer.h>
#include <Foundation/Utilities/AssetFileHeader.h>
#include <GuiFoundation/PropertyGrid/PrefabDefaultStateProvider.h>
#include <GuiFoundation/PropertyGrid/PropertyMetaState.h>
#include <GuiFoundation/UIServices/ImageCache.moc.h>
#include <Texture/Image/ImageConversion.h>

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezAssetDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezAssetDocument::ezAssetDocument(ezStringView sDocumentPath, ezDocumentObjectManager* pObjectManager, ezAssetDocEngineConnection engineConnectionType)
  : ezDocument(sDocumentPath, pObjectManager)
{
  m_EngineConnectionType = engineConnectionType;
  m_EngineStatus = (m_EngineConnectionType != ezAssetDocEngineConnection::None) ? EngineStatus::Disconnected : EngineStatus::Unsupported;
  m_pEngineConnection = nullptr;
  m_uiCommonAssetStateFlags = ezCommonAssetUiState::Grid | ezCommonAssetUiState::Loop | ezCommonAssetUiState::Visualizers;

  if (m_EngineConnectionType != ezAssetDocEngineConnection::None)
  {
    ezEditorEngineProcessConnection::GetSingleton()->s_Events.AddEventHandler(ezMakeDelegate(&ezAssetDocument::EngineConnectionEventHandler, this));
  }
}

ezAssetDocument::~ezAssetDocument()
{
  m_pMirror->DeInit();

  if (m_EngineConnectionType != ezAssetDocEngineConnection::None)
  {
    ezEditorEngineProcessConnection::GetSingleton()->s_Events.RemoveEventHandler(ezMakeDelegate(&ezAssetDocument::EngineConnectionEventHandler, this));

    if (m_pEngineConnection)
    {
      ezEditorEngineProcessConnection::GetSingleton()->DestroyEngineConnection(this);
    }
  }
}

void ezAssetDocument::SetCommonAssetUiState(ezCommonAssetUiState::Enum state, double value)
{
  if (value == 0)
  {
    m_uiCommonAssetStateFlags &= ~((ezUInt32)state);
  }
  else
  {
    m_uiCommonAssetStateFlags |= (ezUInt32)state;
  }

  ezCommonAssetUiState e;
  e.m_State = state;
  e.m_fValue = value;

  m_CommonAssetUiChangeEvent.Broadcast(e);
}

double ezAssetDocument::GetCommonAssetUiState(ezCommonAssetUiState::Enum state) const
{
  return (m_uiCommonAssetStateFlags & (ezUInt32)state) != 0 ? 1.0f : 0.0f;
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
  return GetAssetDocumentTypeDescriptor()->m_AssetDocumentFlags;
}

ezDocumentInfo* ezAssetDocument::CreateDocumentInfo()
{
  return EZ_DEFAULT_NEW(ezAssetDocumentInfo);
}

ezTaskGroupID ezAssetDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  ezAssetDocumentInfo* pInfo = static_cast<ezAssetDocumentInfo*>(m_pDocumentInfo);

  pInfo->m_TransformDependencies.Clear();
  pInfo->m_ThumbnailDependencies.Clear();
  pInfo->m_PackageDependencies.Clear();
  pInfo->m_Outputs.Clear();
  pInfo->m_uiSettingsHash = GetDocumentHash();
  pInfo->m_sAssetsDocumentTypeName.Assign(GetDocumentTypeName());
  pInfo->ClearMetaData();
  UpdateAssetDocumentInfo(pInfo);

  // In case someone added an empty reference.
  pInfo->m_TransformDependencies.Remove(ezString());
  pInfo->m_ThumbnailDependencies.Remove(ezString());
  pInfo->m_PackageDependencies.Remove(ezString());

  return ezDocument::InternalSaveDocument(callback);
}

void ezAssetDocument::InternalAfterSaveDocument()
{
  const auto flags = GetAssetFlags();
  ezAssetCurator::GetSingleton()->NotifyOfFileChange(GetDocumentPath());
  ezAssetCurator::GetSingleton()->MainThreadTick(false);

  if (flags.IsAnySet(ezAssetDocumentFlags::AutoTransformOnSave))
  {
    // If we request an engine connection but the mirror is not set up yet we are still
    // creating the document and TransformAsset will most likely fail.
    if (m_EngineConnectionType == ezAssetDocEngineConnection::None || m_pEngineConnection)
    {
      ezUuid docGuid = GetGuid();

      ezSharedPtr<ezDelegateTask<void>> pTask = EZ_DEFAULT_NEW(ezDelegateTask<void>, "TransformAfterSaveDocument", ezTaskNesting::Never, [docGuid]()
        {
          ezDocument* pDoc = ezDocumentManager::GetDocumentByGuid(docGuid);
          if (pDoc == nullptr)
            return;

          /// \todo Should only be done for platform agnostic assets
          ezTransformStatus ret = ezAssetCurator::GetSingleton()->TransformAsset(docGuid, ezTransformFlags::TriggeredManually);

          if (ret.Failed())
          {
            ezLog::Error("Transform failed: '{0}' ({1})", ret.m_sMessage, pDoc->GetDocumentPath());
          }
          else
          {
            ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();
          }
          //
        });

      pTask->ConfigureTask("TransformAfterSaveDocument", ezTaskNesting::Maybe);
      ezTaskSystem::StartSingleTask(pTask, ezTaskPriority::ThisFrameMainThread);
    }
  }
}

void ezAssetDocument::InitializeAfterLoading(bool bFirstTimeCreation)
{
  m_pMirror = EZ_DEFAULT_NEW(ezIPCObjectMirrorEditor);
}

void ezAssetDocument::InitializeAfterLoadingAndSaving()
{
  if (m_EngineConnectionType != ezAssetDocEngineConnection::None)
  {
    m_pEngineConnection = ezEditorEngineProcessConnection::GetSingleton()->CreateEngineConnection(this);
    m_EngineStatus = EngineStatus::Initializing;

    if (m_EngineConnectionType == ezAssetDocEngineConnection::FullObjectMirroring)
    {
      m_pMirror->SetIPC(m_pEngineConnection);
      m_pMirror->InitSender(GetObjectManager());
    }
  }
}

void ezAssetDocument::AddPrefabDependencies(const ezDocumentObject* pObject, ezAssetDocumentInfo* pInfo) const
{
  {
    const ezDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      ezStringBuilder tmp;
      pInfo->m_TransformDependencies.Insert(ezConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
    }

    m_DocumentObjectMetaData->EndReadMetaData();
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
    const ezDocumentObjectMetaData* pMeta = m_DocumentObjectMetaData->BeginReadMetaData(pObject->GetGuid());

    if (pMeta->m_CreateFromPrefab.IsValid())
    {
      bInsidePrefab = true;
      ezStringBuilder tmp;
      pInfo->m_TransformDependencies.Insert(ezConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
      pInfo->m_ThumbnailDependencies.Insert(ezConversionUtils::ToString(pMeta->m_CreateFromPrefab, tmp));
    }

    m_DocumentObjectMetaData->EndReadMetaData();
  }

  const ezRTTI* pType = pObject->GetTypeAccessor().GetType();
  ezHybridArray<const ezAbstractProperty*, 32> Properties;
  pType->GetAllProperties(Properties);
  for (auto pProp : Properties)
  {
    if (pProp->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
      continue;

    ezBitflags<ezDependencyFlags> depFlags;

    if (auto pAttr = pProp->GetAttributeByType<ezAssetBrowserAttribute>())
    {
      depFlags |= pAttr->GetDependencyFlags();
    }

    if (auto pAttr = pProp->GetAttributeByType<ezFileBrowserAttribute>())
    {
      depFlags |= pAttr->GetDependencyFlags();
    }

    const auto propVarType = pProp->GetSpecificType()->GetVariantType();
    if (propVarType != ezVariantType::String && propVarType != ezVariantType::StringView)
      continue;

    // add all strings that are marked as asset references or file references
    if (depFlags != 0)
    {
      switch (pProp->GetCategory())
      {
        case ezPropertyCategory::Member:
        {
          if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
          {
            if (bInsidePrefab)
            {
              ezHybridArray<ezPropertySelection, 1> selection;
              selection.PushBack({pObject, ezVariant()});
              ezDefaultObjectState defaultState(pType, GetObjectAccessor(), selection.GetArrayPtr());
              if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultValue(pProp))
                continue;
            }

            const ezVariant& value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());

            if (depFlags.IsSet(ezDependencyFlags::Transform))
              pInfo->m_TransformDependencies.Insert(value.Get<ezString>());

            if (depFlags.IsSet(ezDependencyFlags::Thumbnail))
              pInfo->m_ThumbnailDependencies.Insert(value.Get<ezString>());

            if (depFlags.IsSet(ezDependencyFlags::Package))
              pInfo->m_PackageDependencies.Insert(value.Get<ezString>());
          }
        }
        break;

        case ezPropertyCategory::Array:
        case ezPropertyCategory::Set:
        {
          if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
          {
            const ezInt32 iCount = pObject->GetTypeAccessor().GetCount(pProp->GetPropertyName());

            if (bInsidePrefab)
            {
              ezHybridArray<ezPropertySelection, 1> selection;
              selection.PushBack({pObject, ezVariant()});
              ezDefaultContainerState defaultState(pType, GetObjectAccessor(), selection.GetArrayPtr(), pProp->GetPropertyName());
              for (ezInt32 i = 0; i < iCount; ++i)
              {
                ezVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);
                if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultElement(i))
                {
                  continue;
                }
                if (depFlags.IsSet(ezDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(value.Get<ezString>());

                if (depFlags.IsSet(ezDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(value.Get<ezString>());

                if (depFlags.IsSet(ezDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(value.Get<ezString>());
              }
            }
            else
            {
              for (ezInt32 i = 0; i < iCount; ++i)
              {
                ezVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName(), i);

                if (depFlags.IsSet(ezDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(value.Get<ezString>());

                if (depFlags.IsSet(ezDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(value.Get<ezString>());

                if (depFlags.IsSet(ezDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(value.Get<ezString>());
              }
            }
          }
        }
        break;

        case ezPropertyCategory::Map:
          // #TODO Search for exposed params that reference assets.
          if (pProp->GetFlags().IsSet(ezPropertyFlags::StandardType))
          {
            ezVariant value = pObject->GetTypeAccessor().GetValue(pProp->GetPropertyName());
            const ezVariantDictionary& varDict = value.Get<ezVariantDictionary>();
            if (bInsidePrefab)
            {
              ezHybridArray<ezPropertySelection, 1> selection;
              selection.PushBack({pObject, ezVariant()});
              ezDefaultContainerState defaultState(pType, GetObjectAccessor(), selection.GetArrayPtr(), pProp->GetPropertyName());
              for (auto it : varDict)
              {
                if (defaultState.GetStateProviderName() == "Prefab" && defaultState.IsDefaultElement(it.Key()))
                {
                  continue;
                }

                if (depFlags.IsSet(ezDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(it.Value().Get<ezString>());

                if (depFlags.IsSet(ezDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<ezString>());

                if (depFlags.IsSet(ezDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(it.Value().Get<ezString>());
              }
            }
            else
            {
              for (auto it : varDict)
              {
                if (depFlags.IsSet(ezDependencyFlags::Transform))
                  pInfo->m_TransformDependencies.Insert(it.Value().Get<ezString>());

                if (depFlags.IsSet(ezDependencyFlags::Thumbnail))
                  pInfo->m_ThumbnailDependencies.Insert(it.Value().Get<ezString>());

                if (depFlags.IsSet(ezDependencyFlags::Package))
                  pInfo->m_PackageDependencies.Insert(it.Value().Get<ezString>());
              }
            }
          }
          break;

        default:
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

  // Gather used types, sort by name to make it stable and hash their data
  ezSet<const ezRTTI*> types;
  ezToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
  ezDynamicArray<const ezRTTI*> typesSorted;
  typesSorted.Reserve(types.GetCount());
  for (const ezRTTI* pType : types)
  {
    typesSorted.PushBack(pType);
  }

  typesSorted.Sort([](const ezRTTI* a, const ezRTTI* b)
    { return a->GetTypeName().Compare(b->GetTypeName()) < 0; });

  for (const ezRTTI* pType : typesSorted)
  {
    uiHash = ezHashingUtils::xxHash64(pType->GetTypeName().GetStartPointer(), pType->GetTypeName().GetElementCount(), uiHash);
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

ezTransformStatus ezAssetDocument::DoTransformAsset(const ezPlatformProfile* pAssetProfile0 /*= nullptr*/, ezBitflags<ezTransformFlags> transformFlags)
{
  const auto flags = GetAssetFlags();

  if (flags.IsAnySet(ezAssetDocumentFlags::DisableTransform))
    return ezStatus("Asset transform has been disabled on this asset");

  const ezPlatformProfile* pAssetProfile = ezAssetDocumentManager::DetermineFinalTargetProfile(pAssetProfile0);

  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezUInt64 uiPackageHash = 0;
  ezAssetInfo::TransformState state = ezAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), pAssetProfile, GetAssetDocumentTypeDescriptor(), uiHash, uiThumbHash, uiPackageHash);

  if (state == ezAssetInfo::TransformState::UpToDate && !transformFlags.IsSet(ezTransformFlags::ForceTransform))
    return ezStatus(EZ_SUCCESS);

  if (uiHash == 0)
    return ezStatus("Computing the hash for this asset or any dependency failed");

  // Write resource
  {
    ezAssetFileHeader AssetHeader;
    AssetHeader.SetFileHashAndVersion(uiHash, GetAssetTypeVersion());
    const auto& outputs = GetAssetDocumentInfo()->m_Outputs;

    auto GenerateOutput = [this, pAssetProfile, &AssetHeader, transformFlags](const char* szOutputTag) -> ezTransformStatus
    {
      const ezString sTargetFile = GetAssetDocumentManager()->GetAbsoluteOutputFileName(GetAssetDocumentTypeDescriptor(), GetDocumentPath(), szOutputTag, pAssetProfile);
      ezTransformStatus ret = InternalTransformAsset(sTargetFile, szOutputTag, pAssetProfile, AssetHeader, transformFlags);

      // if writing failed, make sure the output file does not exist
      if (ret.Failed())
      {
        ezFileSystem::DeleteFile(sTargetFile);
      }
      ezAssetCurator::GetSingleton()->NotifyOfFileChange(sTargetFile);
      return ret;
    };

    ezTransformStatus res;
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

ezTransformStatus ezAssetDocument::TransformAsset(ezBitflags<ezTransformFlags> transformFlags, const ezPlatformProfile* pAssetProfile)
{
  EZ_PROFILE_SCOPE("TransformAsset");

  if (!transformFlags.IsSet(ezTransformFlags::ForceTransform))
  {
    EZ_SUCCEED_OR_RETURN(SaveDocument());

    const auto assetFlags = GetAssetFlags();

    if (assetFlags.IsSet(ezAssetDocumentFlags::DisableTransform) || (assetFlags.IsSet(ezAssetDocumentFlags::OnlyTransformManually) && !transformFlags.IsSet(ezTransformFlags::TriggeredManually)))
    {
      return ezStatus(EZ_SUCCESS);
    }
  }

  const ezTransformStatus res = DoTransformAsset(pAssetProfile, transformFlags);

  if (transformFlags.IsSet(ezTransformFlags::TriggeredManually))
  {
    SaveDocument().LogFailure();
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
  }

  return res;
}

ezTransformStatus ezAssetDocument::CreateThumbnail()
{
  ezUInt64 uiHash = 0;
  ezUInt64 uiThumbHash = 0;
  ezUInt64 uiPackageHash = 0;

  ezAssetInfo::TransformState state = ezAssetCurator::GetSingleton()->IsAssetUpToDate(GetGuid(), ezAssetCurator::GetSingleton()->GetActiveAssetProfile(), GetAssetDocumentTypeDescriptor(), uiHash, uiThumbHash, uiPackageHash);

  if (state == ezAssetInfo::TransformState::UpToDate)
    return ezStatus(EZ_SUCCESS);

  if (uiHash == 0)
    return ezStatus("Computing the hash for this asset or any dependency failed");

  if (state == ezAssetInfo::NeedsThumbnail)
  {
    ThumbnailInfo ThumbnailInfo;
    ThumbnailInfo.SetFileHashAndVersion(uiThumbHash, GetAssetTypeVersion());
    ezTransformStatus res = InternalCreateThumbnail(ThumbnailInfo);

    InvalidateAssetThumbnail();
    ezAssetCurator::GetSingleton()->NotifyOfAssetChange(GetGuid());
    return res;
  }
  return ezTransformStatus(ezFmt("Asset state is {}", state));
}

ezTransformStatus ezAssetDocument::InternalTransformAsset(const char* szTargetFile, ezStringView sOutputTag, const ezPlatformProfile* pAssetProfile, const ezAssetFileHeader& AssetHeader, ezBitflags<ezTransformFlags> transformFlags)
{
  ezDeferredFileWriter file;
  file.SetOutput(szTargetFile);

  if (AssetHeader.Write(file) == EZ_FAILURE)
  {
    file.Discard();
    return ezTransformStatus("Failed to write asset header");
  }

  ezTransformStatus res = InternalTransformAsset(file, sOutputTag, pAssetProfile, AssetHeader, transformFlags);
  if (res.m_Result != ezTransformResult::Success)
  {
    // We do not want to overwrite the old output file if we failed to transform the asset.
    file.Discard();
    return res;
  }


  if (file.Close().Failed())
  {
    ezLog::Error("Could not open file for writing: '{0}'", szTargetFile);
    return ezStatus("Opening the asset output file failed");
  }

  return ezStatus(EZ_SUCCESS);
}

ezString ezAssetDocument::GetThumbnailFilePath(ezStringView sSubAssetName /*= ezStringView()*/) const
{
  return GetAssetDocumentManager()->GenerateResourceThumbnailPath(GetDocumentPath(), sSubAssetName);
}

void ezAssetDocument::InvalidateAssetThumbnail(ezStringView sSubAssetName /*= ezStringView()*/) const
{
  const ezString sResourceFile = GetThumbnailFilePath(sSubAssetName);
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
    if (qimg.width() != ezThumbnailSize)
      qimg = qimg.scaled(ezThumbnailSize, ezThumbnailSize, Qt::AspectRatioMode::IgnoreAspectRatio, Qt::TransformationMode::SmoothTransformation);
  }
  else
  {
    // center the image in a square canvas

    // scale the longer edge to ezThumbnailSize
    if (qimg.width() > qimg.height())
      qimg = qimg.scaledToWidth(ezThumbnailSize, Qt::TransformationMode::SmoothTransformation);
    else
      qimg = qimg.scaledToHeight(ezThumbnailSize, Qt::TransformationMode::SmoothTransformation);

    // create a black canvas
    QImage img2(ezThumbnailSize, ezThumbnailSize, QImage::Format_RGBA8888);
    img2.fill(Qt::GlobalColor::black);

    QPoint destPos = QPoint((ezThumbnailSize - qimg.width()) / 2, (ezThumbnailSize - qimg.height()) / 2);

    // paint the smaller image such that it ends up centered
    QPainter painter(&img2);
    painter.drawImage(destPos, qimg);
    painter.end();

    qimg = img2;
  }

  // make sure the directory exists, Qt will not create sub-folders
  const ezStringBuilder sDir = sResourceFile.GetFileDirectory();
  EZ_SUCCEED_OR_RETURN(ezOSFile::CreateDirectoryStructure(sDir));

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

void ezAssetDocument::AppendThumbnailInfo(ezStringView sThumbnailFile, const ThumbnailInfo& thumbnailInfo) const
{
  ezContiguousMemoryStreamStorage storage;
  {
    ezFileReader reader;
    if (reader.Open(sThumbnailFile).Failed())
    {
      return;
    }
    storage.ReadAll(reader);
  }

  ezDeferredFileWriter writer;
  writer.SetOutput(sThumbnailFile);
  writer.WriteBytes(storage.GetData(), storage.GetStorageSize64()).IgnoreResult();

  thumbnailInfo.Serialize(writer).IgnoreResult();

  if (writer.Close().Failed())
  {
    ezLog::Error("Could not open file for writing: '{0}'", sThumbnailFile);
  }
}

ezStatus ezAssetDocument::RemoteExport(const ezAssetFileHeader& header, const char* szOutputTarget) const
{
  ezProgressRange range("Exporting Asset", 2, false);

  ezLog::Info("Exporting {0} to \"{1}\"", GetDocumentTypeName(), szOutputTarget);

  EZ_SUCCEED_OR_RETURN(WaitForEngineStatusLoaded());

  range.BeginNextStep(szOutputTarget);

  ezExportDocumentMsgToEngine msg;
  msg.m_sOutputFile = szOutputTarget;
  msg.m_uiAssetHash = header.GetFileHash();
  msg.m_uiVersion = header.GetFileVersion();

  GetEditorEngineConnection()->SendMessage(&msg);

  ezStatus status(EZ_SUCCESS);
  ezProcessCommunicationChannel::WaitForMessageCallback callback = [&status](ezProcessMessage* pMsg) -> bool
  {
    ezExportDocumentMsgToEditor* pMsg2 = ezDynamicCast<ezExportDocumentMsgToEditor*>(pMsg);

    if (!pMsg2->m_bOutputSuccess)
      status = ezStatus(pMsg2->m_sFailureMsg.GetView());

    return true;
  };

  if (ezEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), ezExportDocumentMsgToEditor::GetStaticRTTI(), ezTime::MakeFromSeconds(60), &callback).Failed())
  {
    return ezStatus(ezFmt("Remote exporting {0} to \"{1}\" timed out.", GetDocumentTypeName(), msg.m_sOutputFile));
  }
  else
  {
    if (status.Failed())
    {
      return status;
    }

    ezLog::Success("{0} \"{1}\" has been exported.", GetDocumentTypeName(), msg.m_sOutputFile);

    ShowDocumentStatus(ezFmt("{0} exported successfully", GetDocumentTypeName()));

    return ezStatus(EZ_SUCCESS);
  }
}

ezTransformStatus ezAssetDocument::InternalCreateThumbnail(const ThumbnailInfo& thumbnailInfo)
{
  EZ_ASSERT_NOT_IMPLEMENTED;
  return ezStatus("Not implemented");
}

ezStatus ezAssetDocument::RemoteCreateThumbnail(const ThumbnailInfo& thumbnailInfo, ezArrayPtr<ezStringView> viewExclusionTags) const
{
  ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  ezLog::Info("Create {0} thumbnail for \"{1}\"", GetDocumentTypeName(), GetDocumentPath());

  if (GetEngineStatus() == ezAssetDocument::EngineStatus::Disconnected)
  {
    return ezStatus(ezFmt("Create {0} thumbnail for \"{1}\" failed, engine not started or crashed.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else if (GetEngineStatus() == ezAssetDocument::EngineStatus::Initializing)
  {
    if (ezEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), ezTime::MakeFromSeconds(10)).Failed())
    {
      return ezStatus(ezFmt("Create {0} thumbnail for \"{1}\" failed, document initialization timed out.", GetDocumentTypeName(), GetDocumentPath()));
    }
    EZ_ASSERT_DEV(GetEngineStatus() == ezAssetDocument::EngineStatus::Loaded, "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }

  SyncObjectsToEngine();
  ezCreateThumbnailMsgToEngine msg;
  msg.m_uiWidth = ezThumbnailSize;
  msg.m_uiHeight = ezThumbnailSize;
  for (const ezStringView& tag : viewExclusionTags)
  {
    msg.m_ViewExcludeTags.PushBack(tag);
  }
  GetEditorEngineConnection()->SendMessage(&msg);

  ezDataBuffer data;
  ezProcessCommunicationChannel::WaitForMessageCallback callback = [&data](ezProcessMessage* pMsg) -> bool
  {
    ezCreateThumbnailMsgToEditor* pThumbnailMsg = ezDynamicCast<ezCreateThumbnailMsgToEditor*>(pMsg);
    data = pThumbnailMsg->m_ThumbnailData;
    return true;
  };

  if (ezEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), ezCreateThumbnailMsgToEditor::GetStaticRTTI(), ezTime::MakeFromSeconds(60), &callback).Failed())
  {
    return ezStatus(ezFmt("Create {0} thumbnail for \"{1}\" failed timed out.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else
  {
    if (data.GetCount() != msg.m_uiWidth * msg.m_uiHeight * 4)
    {
      return ezStatus(ezFmt("Thumbnail generation for {0} failed, thumbnail data is empty.", GetDocumentTypeName()));
    }

    ezImageHeader imgHeader;
    imgHeader.SetImageFormat(ezImageFormat::R8G8B8A8_UNORM);
    imgHeader.SetWidth(msg.m_uiWidth);
    imgHeader.SetHeight(msg.m_uiHeight);

    ezImage image;
    image.ResetAndAlloc(imgHeader);
    EZ_ASSERT_DEV(data.GetCount() == imgHeader.ComputeDataSize(), "Thumbnail ezImage has different size than data buffer!");
    ezMemoryUtils::Copy(image.GetPixelPointer<ezUInt8>(), data.GetData(), msg.m_uiWidth * msg.m_uiHeight * 4);
    SaveThumbnail(image, thumbnailInfo).LogFailure();

    ezLog::Success("{0} thumbnail for \"{1}\" has been exported.", GetDocumentTypeName(), GetDocumentPath());

    ShowDocumentStatus(ezFmt("{0} thumbnail created successfully", GetDocumentTypeName()));

    return ezStatus(EZ_SUCCESS);
  }
}

ezUInt16 ezAssetDocument::GetAssetTypeVersion() const
{
  return (ezUInt16)GetDynamicRTTI()->GetTypeVersion();
}


ezStatus ezAssetDocument::WaitForEngineStatusLoaded() const
{
  if (GetEngineStatus() == ezAssetDocument::EngineStatus::Disconnected)
  {
    return ezStatus(ezFmt("Loading {0} document '{1}' failed, engine not started or crashed.", GetDocumentTypeName(), GetDocumentPath()));
  }
  else if (GetEngineStatus() == ezAssetDocument::EngineStatus::Initializing)
  {
    if (ezEditorEngineProcessConnection::GetSingleton()->WaitForDocumentMessage(GetGuid(), ezDocumentOpenResponseMsgToEditor::GetStaticRTTI(), {}).Failed())
    {
      return ezStatus(ezFmt("Loading {0} document '{1}' failed, document initialization timed out or engine crashed.", GetDocumentTypeName(), GetDocumentPath()));
    }
    EZ_ASSERT_DEV(GetEngineStatus() == ezAssetDocument::EngineStatus::Loaded, "After receiving ezDocumentOpenResponseMsgToEditor, the document should be in loaded state.");
  }
  return ezStatus(EZ_SUCCESS);
}

bool ezAssetDocument::SendMessageToEngine(ezEditorEngineDocumentMsg* pMessage /*= false*/) const
{
  return GetEditorEngineConnection()->SendMessage(pMessage);
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

      m_pMirror->SendDocument();
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
    { RemoveSyncObject(pSync); });

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

ezEditorEngineSyncObject* ezAssetDocument::FindSyncObject(const ezRTTI* pType) const
{
  for (ezEditorEngineSyncObject* pSync : m_SyncObjects)
  {
    if (pSync->GetDynamicRTTI() == pType)
    {
      return pSync;
    }
  }
  return nullptr;
}

void ezAssetDocument::SyncObjectsToEngine() const
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

    ezContiguousMemoryStreamStorage storage;
    ezMemoryStreamWriter writer(&storage);
    ezMemoryStreamReader reader(&storage);

    ezReflectionSerializer::WriteObjectToBinary(writer, pObject->GetDynamicRTTI(), pObject);
    msg.m_ObjectData = ezArrayPtr<const ezUInt8>(storage.GetData(), storage.GetStorageSize32());

    SendMessageToEngine(&msg);

    pObject->SetModified(false);
  }
}

void ezAssetDocument::SendDocumentOpenMessage(bool bOpen)
{
  EZ_PROFILE_SCOPE("SendDocumentOpenMessage");

  // it is important to have up-to-date lookup tables in the engine process, because document contexts might try to
  // load resources, and if the file redirection does not happen correctly, derived resource types may not be created as they should
  ezAssetCurator::GetSingleton()->WriteAssetTables().IgnoreResult();

  m_EngineStatus = EngineStatus::Initializing;

  ezDocumentOpenMsgToEngine m;
  m.m_DocumentGuid = GetGuid();
  m.m_bDocumentOpen = bOpen;
  m.m_sDocumentType = GetDocumentTypeDescriptor()->m_sDocumentTypeName;
  m.m_DocumentMetaData = GetCreateEngineMetaData();

  if (!ezEditorEngineProcessConnection::GetSingleton()->SendMessage(&m))
  {
    ezLog::Error("Failed to send DocumentOpenMessage");
  }
}

namespace
{
  static const char* szThumbnailInfoTag = "ezThumb";
}

ezResult ezAssetDocument::ThumbnailInfo::Deserialize(ezStreamReader& inout_reader)
{
  char tag[8] = {0};

  if (inout_reader.ReadBytes(tag, 7) != 7)
    return EZ_FAILURE;

  if (!ezStringUtils::IsEqual(tag, szThumbnailInfoTag))
  {
    return EZ_FAILURE;
  }

  inout_reader >> m_uiHash;
  inout_reader >> m_uiVersion;
  inout_reader >> m_uiReserved;

  return EZ_SUCCESS;
}

ezResult ezAssetDocument::ThumbnailInfo::Serialize(ezStreamWriter& inout_writer) const
{
  EZ_SUCCEED_OR_RETURN(inout_writer.WriteBytes(szThumbnailInfoTag, 7));

  inout_writer << m_uiHash;
  inout_writer << m_uiVersion;
  inout_writer << m_uiReserved;

  return EZ_SUCCESS;
}
