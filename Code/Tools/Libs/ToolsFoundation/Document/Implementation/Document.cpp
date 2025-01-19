#include <ToolsFoundation/ToolsFoundationPCH.h>

#include <Foundation/IO/FileSystem/DeferredFileWriter.h>
#include <Foundation/IO/FileSystem/FileReader.h>
#include <Foundation/IO/FileSystem/FileWriter.h>
#include <Foundation/IO/MemoryStream.h>
#include <Foundation/Profiling/Profiling.h>
#include <Foundation/Serialization/DdlSerializer.h>
#include <Foundation/Serialization/RttiConverter.h>
#include <Foundation/Threading/TaskSystem.h>
#include <Foundation/Time/Stopwatch.h>
#include <Foundation/Utilities/Progress.h>
#include <ToolsFoundation/Command/TreeCommands.h>
#include <ToolsFoundation/Document/Document.h>
#include <ToolsFoundation/Document/DocumentManager.h>
#include <ToolsFoundation/Document/DocumentTasks.h>
#include <ToolsFoundation/Document/PrefabCache.h>
#include <ToolsFoundation/Document/PrefabUtils.h>
#include <ToolsFoundation/Object/ObjectCommandAccessor.h>
#include <ToolsFoundation/Serialization/DocumentObjectConverter.h>
#include <ToolsFoundation/Serialization/ToolsSerializationUtils.h>

// clang-format off
EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentObjectMetaData, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    //EZ_MEMBER_PROPERTY("MetaHidden", m_bHidden) // remove this property to disable serialization
    EZ_MEMBER_PROPERTY("MetaFromPrefab", m_CreateFromPrefab),
    EZ_MEMBER_PROPERTY("MetaPrefabSeed", m_PrefabSeedGuid),
    EZ_MEMBER_PROPERTY("MetaBasePrefab", m_sBasePrefab),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;

EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocumentInfo, 1, ezRTTINoAllocator)
{
  EZ_BEGIN_PROPERTIES
  {
    EZ_MEMBER_PROPERTY("DocumentID", m_DocumentID),
  }
  EZ_END_PROPERTIES;
}
EZ_END_DYNAMIC_REFLECTED_TYPE;
// clang-format on

ezDocumentInfo::ezDocumentInfo()
{
  m_DocumentID = ezUuid::MakeUuid();
}


EZ_BEGIN_DYNAMIC_REFLECTED_TYPE(ezDocument, 1, ezRTTINoAllocator)
EZ_END_DYNAMIC_REFLECTED_TYPE;

ezEvent<const ezDocumentEvent&> ezDocument::s_EventsAny;

ezDocument::ezDocument(ezStringView sPath, ezDocumentObjectManager* pDocumentObjectManagerImpl)
{
  using ObjectMetaData = ezObjectMetaData<ezUuid, ezDocumentObjectMetaData>;
  m_DocumentObjectMetaData = EZ_DEFAULT_NEW(ObjectMetaData);
  m_pDocumentInfo = nullptr;
  m_sDocumentPath = sPath;
  m_pObjectManager = ezUniquePtr<ezDocumentObjectManager>(pDocumentObjectManagerImpl, ezFoundation::GetDefaultAllocator());
  m_pObjectManager->SetDocument(this);
  m_pCommandHistory = EZ_DEFAULT_NEW(ezCommandHistory, this);
  m_pSelectionManager = EZ_DEFAULT_NEW(ezSelectionManager, m_pObjectManager.Borrow());

  if (m_pObjectAccessor == nullptr)
  {
    m_pObjectAccessor = EZ_DEFAULT_NEW(ezObjectCommandAccessor, m_pCommandHistory.Borrow());
  }

  m_bWindowRequested = false;
  m_bModified = true;
  m_bReadOnly = false;
  m_bAddToRecentFilesList = true;

  m_uiUnknownObjectTypeInstances = 0;

  m_pHostDocument = this;
  m_pActiveSubDocument = this;
}

ezDocument::~ezDocument()
{
  m_pSelectionManager = nullptr;

  m_pObjectManager->DestroyAllObjects();

  m_pCommandHistory->ClearRedoHistory();
  m_pCommandHistory->ClearUndoHistory();

  EZ_DEFAULT_DELETE(m_pDocumentInfo);
}

void ezDocument::SetupDocumentInfo(const ezDocumentTypeDescriptor* pTypeDescriptor)
{
  m_pTypeDescriptor = pTypeDescriptor;
  m_pDocumentInfo = CreateDocumentInfo();

  EZ_ASSERT_DEV(m_pDocumentInfo != nullptr, "invalid document info");
}

void ezDocument::SetModified(bool b)
{
  if (m_bModified == b)
    return;

  m_bModified = b;

  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::ModifiedChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void ezDocument::SetReadOnly(bool b)
{
  if (m_bReadOnly == b)
    return;

  m_bReadOnly = b;

  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::ReadOnlyChanged;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezStatus ezDocument::SaveDocument(bool bForce)
{
  if (!IsModified() && !bForce)
    return ezStatus(EZ_SUCCESS);

  // In the unlikely event that we manage to edit a doc and call save again while
  // an async save is already in progress we block on the first save to ensure
  // the correct chronological state on disk after both save ops are done.
  if (m_ActiveSaveTask.IsValid())
  {
    ezTaskSystem::WaitForGroup(m_ActiveSaveTask);
    m_ActiveSaveTask.Invalidate();
  }
  ezStatus result(EZ_SUCCESS);

  m_ActiveSaveTask = InternalSaveDocument([&result](ezDocument* pDoc, ezStatus res)
    { result = res; });

  ezTaskSystem::WaitForGroup(m_ActiveSaveTask);
  m_ActiveSaveTask.Invalidate();

  return result;
}


ezTaskGroupID ezDocument::SaveDocumentAsync(AfterSaveCallback callback, bool bForce)
{
  if (!IsModified() && !bForce)
    return ezTaskGroupID();

  m_ActiveSaveTask = InternalSaveDocument(callback);
  return m_ActiveSaveTask;
}

void ezDocument::DocumentRenamed(ezStringView sNewDocumentPath)
{
  m_sDocumentPath = sNewDocumentPath;

  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::DocumentRenamed;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

void ezDocument::EnsureVisible()
{
  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_Type = ezDocumentEvent::Type::EnsureVisible;

  m_EventsOne.Broadcast(e);
  s_EventsAny.Broadcast(e);
}

ezTaskGroupID ezDocument::InternalSaveDocument(AfterSaveCallback callback)
{
  EZ_PROFILE_SCOPE("InternalSaveDocument");
  ezTaskGroupID saveID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::LongRunningHighPriority);
  auto saveTask = EZ_DEFAULT_NEW(ezSaveDocumentTask);

  {
    saveTask->m_document = this;
    saveTask->file.SetOutput(m_sDocumentPath);
    ezTaskSystem::AddTaskToGroup(saveID, saveTask);

    {
      ezRttiConverterContext context;
      ezRttiConverterWriter rttiConverter(&saveTask->headerGraph, &context, true, true);
      context.RegisterObject(GetGuid(), m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
      rttiConverter.AddObjectToGraph(m_pDocumentInfo, "Header");
    }
    {
      // Do not serialize any temporary properties into the document.
      auto filter = [](const ezDocumentObject*, const ezAbstractProperty* pProp) -> bool
      {
        if (pProp->GetAttributeByType<ezTemporaryAttribute>() != nullptr)
          return false;
        return true;
      };
      ezDocumentObjectConverterWriter objectConverter(&saveTask->objectGraph, GetObjectManager(), filter);
      objectConverter.AddObjectToGraph(GetObjectManager()->GetRootObject(), "ObjectTree");

      AttachMetaDataBeforeSaving(saveTask->objectGraph);
    }
    {
      ezSet<const ezRTTI*> types;
      ezToolsReflectionUtils::GatherObjectTypes(GetObjectManager()->GetRootObject(), types);
      ezToolsSerializationUtils::SerializeTypes(types, saveTask->typesGraph);
    }
  }

  ezTaskGroupID afterSaveID = ezTaskSystem::CreateTaskGroup(ezTaskPriority::SomeFrameMainThread);
  {
    auto afterSaveTask = EZ_DEFAULT_NEW(ezAfterSaveDocumentTask);
    afterSaveTask->m_document = this;
    afterSaveTask->m_callback = callback;
    ezTaskSystem::AddTaskToGroup(afterSaveID, afterSaveTask);
  }
  ezTaskSystem::AddTaskGroupDependency(afterSaveID, saveID);
  if (!ezTaskSystem::IsTaskGroupFinished(m_ActiveSaveTask))
  {
    ezTaskSystem::AddTaskGroupDependency(saveID, m_ActiveSaveTask);
  }

  ezTaskSystem::StartTaskGroup(saveID);
  ezTaskSystem::StartTaskGroup(afterSaveID);
  return afterSaveID;
}

ezStatus ezDocument::ReadDocument(ezStringView sDocumentPath, ezUniquePtr<ezAbstractObjectGraph>& ref_pHeader, ezUniquePtr<ezAbstractObjectGraph>& ref_pObjects,
  ezUniquePtr<ezAbstractObjectGraph>& ref_pTypes)
{
  ezDefaultMemoryStreamStorage storage;
  ezMemoryStreamReader memreader(&storage);

  {
    EZ_PROFILE_SCOPE("Read File");
    ezFileReader file;
    if (file.Open(sDocumentPath) == EZ_FAILURE)
    {
      return ezStatus("Unable to open file for reading!");
    }

    // range.BeginNextStep("Reading File");
    storage.ReadAll(file);

    // range.BeginNextStep("Parsing Graph");
    {
      EZ_PROFILE_SCOPE("parse DDL graph");
      ezStopwatch sw;
      if (ezAbstractGraphDdlSerializer::ReadDocument(memreader, ref_pHeader, ref_pObjects, ref_pTypes, true).Failed())
        return ezStatus("Failed to parse DDL graph");

      ezTime t = sw.GetRunningTotal();
      ezLog::Debug("DDL parsing time: {0} msec", ezArgF(t.GetMilliseconds(), 1));
    }
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDocument::ReadAndRegisterTypes(const ezAbstractObjectGraph& types)
{
  EZ_PROFILE_SCOPE("Deserializing Types");
  // range.BeginNextStep("Deserializing Types");

  // Deserialize and register serialized phantom types.
  ezString sDescTypeName = ezGetStaticRTTI<ezReflectedTypeDescriptor>()->GetTypeName();
  ezDynamicArray<ezReflectedTypeDescriptor*> descriptors;
  auto& nodes = types.GetAllNodes();
  descriptors.Reserve(nodes.GetCount()); // Overkill but doesn't matter much as it's just temporary.
  ezRttiConverterContext context;
  ezRttiConverterReader rttiConverter(&types, &context);

  for (auto it = nodes.GetIterator(); it.IsValid(); ++it)
  {
    if (it.Value()->GetType() == sDescTypeName)
    {
      ezReflectedTypeDescriptor* pDesc = rttiConverter.CreateObjectFromNode(it.Value()).Cast<ezReflectedTypeDescriptor>();
      if (pDesc->m_Flags.IsSet(ezTypeFlags::Minimal))
      {
        ezGetStaticRTTI<ezReflectedTypeDescriptor>()->GetAllocator()->Deallocate(pDesc);
      }
      else
      {
        descriptors.PushBack(pDesc);
      }
    }
  }
  ezToolsReflectionUtils::DependencySortTypeDescriptorArray(descriptors);
  for (ezReflectedTypeDescriptor* desc : descriptors)
  {
    if (!ezRTTI::FindTypeByName(desc->m_sTypeName))
    {
      ezPhantomRttiManager::RegisterType(*desc);
    }
    ezGetStaticRTTI<ezReflectedTypeDescriptor>()->GetAllocator()->Deallocate(desc);
  }
  return ezStatus(EZ_SUCCESS);
}

ezStatus ezDocument::InternalLoadDocument()
{
  EZ_PROFILE_SCOPE("InternalLoadDocument");
  // this would currently crash in Qt, due to the processEvents in the QtProgressBar
  // ezProgressRange range("Loading Document", 5, false);

  ezUniquePtr<ezAbstractObjectGraph> header;
  ezUniquePtr<ezAbstractObjectGraph> objects;
  ezUniquePtr<ezAbstractObjectGraph> types;

  ezStatus res = ReadDocument(m_sDocumentPath, header, objects, types);
  if (res.Failed())
    return res;

  res = ReadAndRegisterTypes(*types.Borrow());
  if (res.Failed())
    return res;

  {
    EZ_PROFILE_SCOPE("Restoring Header");
    ezRttiConverterContext context;
    ezRttiConverterReader rttiConverter(header.Borrow(), &context);
    auto* pHeaderNode = header->GetNodeByName("Header");
    rttiConverter.ApplyPropertiesToObject(pHeaderNode, m_pDocumentInfo->GetDynamicRTTI(), m_pDocumentInfo);
  }

  {
    EZ_PROFILE_SCOPE("Restoring Objects");
    ezDocumentObjectConverterReader objectConverter(
      objects.Borrow(), GetObjectManager(), ezDocumentObjectConverterReader::Mode::CreateAndAddToDocument);
    // range.BeginNextStep("Restoring Objects");
    auto* pRootNode = objects->GetNodeByName("ObjectTree");
    objectConverter.ApplyPropertiesToObject(pRootNode, GetObjectManager()->GetRootObject());

    SetUnknownObjectTypes(objectConverter.GetUnknownObjectTypes(), objectConverter.GetNumUnknownObjectCreations());
  }

  {
    EZ_PROFILE_SCOPE("Restoring Meta-Data");
    // range.BeginNextStep("Restoring Meta-Data");
    RestoreMetaDataAfterLoading(*objects.Borrow(), false);
  }

  SetModified(false);
  return ezStatus(EZ_SUCCESS);
}

void ezDocument::AttachMetaDataBeforeSaving(ezAbstractObjectGraph& graph) const
{
  m_DocumentObjectMetaData->AttachMetaDataToAbstractGraph(graph);
}

void ezDocument::RestoreMetaDataAfterLoading(const ezAbstractObjectGraph& graph, bool bUndoable)
{
  m_DocumentObjectMetaData->RestoreMetaDataFromAbstractGraph(graph);
}

void ezDocument::BeforeClosing()
{
  // This can't be done in the dtor as the task uses virtual functions on this object.
  if (m_ActiveSaveTask.IsValid())
  {
    ezTaskSystem::WaitForGroup(m_ActiveSaveTask);
    m_ActiveSaveTask.Invalidate();
  }
}

void ezDocument::SetUnknownObjectTypes(const ezSet<ezString>& Types, ezUInt32 uiInstances)
{
  m_UnknownObjectTypes = Types;
  m_uiUnknownObjectTypeInstances = uiInstances;
}


void ezDocument::BroadcastInterDocumentMessage(ezReflectedClass* pMessage, ezDocument* pSender)
{
  for (auto& man : ezDocumentManager::GetAllDocumentManagers())
  {
    for (auto pDoc : man->GetAllOpenDocuments())
    {
      if (pDoc == pSender)
        continue;

      pDoc->OnInterDocumentMessage(pMessage, pSender);
    }
  }
}

void ezDocument::DeleteSelectedObjects() const
{
  ezHybridArray<ezSelectionEntry, 64> objects;
  GetSelectionManager()->GetTopLevelSelection(objects);

  // make sure the whole selection is cleared, otherwise each delete command would reduce the selection one by one
  GetSelectionManager()->Clear();

  auto history = GetCommandHistory();
  history->StartTransaction("Delete Object");

  ezRemoveObjectCommand cmd;

  for (const ezSelectionEntry& entry : objects)
  {
    cmd.m_Object = entry.m_pObject->GetGuid();

    if (history->AddCommand(cmd).Failed())
    {
      history->CancelTransaction();
      return;
    }
  }

  history->FinishTransaction();
}

void ezDocument::ShowDocumentStatus(const ezFormatString& msg) const
{
  ezStringBuilder tmp;

  ezDocumentEvent e;
  e.m_pDocument = this;
  e.m_sStatusMsg = msg.GetText(tmp);
  e.m_Type = ezDocumentEvent::Type::DocumentStatusMsg;

  m_EventsOne.Broadcast(e);
}


ezResult ezDocument::ComputeObjectTransformation(const ezDocumentObject* pObject, ezTransform& out_result) const
{
  out_result.SetIdentity();
  return EZ_FAILURE;
}

ezObjectAccessorBase* ezDocument::GetObjectAccessor() const
{
  return m_pObjectAccessor.Borrow();
}
